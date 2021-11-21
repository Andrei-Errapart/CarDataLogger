/**
vim: ts=4
vim: shiftwidth=4
*/

#include <stdint.h>
#include "LoggerConfig.h"
#include "LoggerIO.h"
#include "Gps.h"
#include "AccelerationSensors.h"
#include "Display.h"
#include "Utils.h"
#include "IClock.h"
#include "IUsart.h"
#include "tprintf.h"
#include <Filesystem/Blockdevice_SDMMC.h>
#include <Filesystem/FAT16.h>
#include <Filesystem/File.h>
#include <Filesystem/Endian.h>		// FixEndian32
#include "project.h"

#include <led.h>

//*******************************************************************
#define sdram_check(sdram,i, v) do {	\
	sdram[i]= (v);						\
	if (sdram[i] != (v)) {				\
		goto sdram_failure;				\
	}									\
} while (0)

//*******************************************************************
static void
SDRAM_Init(void)
{
	tprintf("SDRAM...");

	const unsigned int	sdram_size = SDRAM_SIZE >> 2;
	const unsigned int	progress_inc = 1024*1024 / 4;
	unsigned int		progress_i = 0;
	unsigned int		progress_mb = 0;
	unsigned int		i;
	volatile uint32_t*	sdram = reinterpret_cast<volatile uint32_t*>(SDRAM);
	char				xbuf[32];

	// Initialize the external SDRAM chip.
	sdramc_init(F_CPU);
	tprintf(" doh ");

	// Display SDRAM size.
	dip204_set_cursor_position(1,1);
	dip204_write_string("CPU:");
	dip204_write_string(itoa(F_CPU/1000000, xbuf, 10));
	dip204_write_string("MHz MEM:");
	dip204_write_string(itoa(sdram_size/progress_inc, xbuf, 10));
	dip204_write_string("MB");


	if (true) {
		delay_ms(600);
	} else {
		for (i=0; i<sdram_size; ++i) {
			sdram_check(sdram,i,0);
			sdram_check(sdram,i,0xFFFFFFFF);
			sdram_check(sdram,i,0xAAAAAAAA);
			sdram_check(sdram,i,0x55555555);
			if (i == progress_i) {
				dip204_set_cursor_position(1,2);
				dip204_write_string("Checking: ");
				if (progress_mb<10) {
					dip204_write_string(" ");
				}
				dip204_write_string(itoa(progress_mb, xbuf, 10));
				dip204_write_string("MB");
				progress_i += progress_inc;
				progress_mb += 1;
			}
			continue;
	sdram_failure:
			dip204_set_cursor_position(1,2);
			dip204_write_string("MEMORY ERROR!");
			dip204_set_cursor_position(1,3);
			dip204_write_string("Addr: 0x");
			dip204_write_string(itoa(i, xbuf, 16));
			for (;;);
		}
	}

	tprintf(" done.\n");
}

//*******************************************************************
static void
FixEndianHELLO(
	LoggerIO::HELLO&	packet
)
{
	Filesystem::FixEndian32(packet.Magic);
	Filesystem::FixEndian32(packet.Frequency);
	Filesystem::FixEndian32(packet.Tick);
}

//*******************************************************************
static void
FixEndianGPS(
	LoggerIO::GPS&	packet
)
{
	Filesystem::FixEndian16(packet.Header.Type);
	Filesystem::FixEndian16(packet.Header.TotalSize);
	Filesystem::FixEndian32(packet.Header.Tick);
}

//*******************************************************************
static void
FixEndianSENSORS(
	LoggerIO::SENSORS&	packet
)
{
	Filesystem::FixEndian16(packet.Header.Type);
	Filesystem::FixEndian16(packet.Header.TotalSize);
	Filesystem::FixEndian32(packet.Header.Tick);
}

//*******************************************************************
static void
memorycard_loop()
{
	const char*						filename = "LOGGER.BIN";

	Filesystem::Blockdevice_SDMMC	sdmmc_card;
	Filesystem::FAT16				filesys(sdmmc_card);
	Filesystem::File				f(filesys, filename, Filesystem::OPEN_CREATE);
	const unsigned int				filesize = f.Size();

	f.SeekSet(filesize);	// prepare for append.

	Display_MemoryCard("Writing LOGGER.BIN.");
	Display_Error("");
	Display_Draw();

	// Hello packet.
	{
		LoggerIO::HELLO			PacketHELLO;
		PacketHELLO.Magic 		= LoggerIO::MAGIC;
		PacketHELLO.Frequency	= F_CPU;
		PacketHELLO.Tick		= AccelerationSensors_GetTick();
		FixEndianHELLO(PacketHELLO);

		f.Write(&PacketHELLO, sizeof(PacketHELLO));

		Display_Sleep(2 * 1000 / LoggerConfig::SamplingFrequency);
	}


	const unsigned int	interval_packets = LoggerConfig::WritingInterval * LoggerConfig::SamplingFrequency;
	const unsigned int	threshold = interval_packets + 
										(LoggerConfig::LimitsTimeBefore + LoggerConfig::LimitsTimeAfter) * LoggerConfig::SamplingFrequency;
	const unsigned int	before_packets = LoggerConfig::LimitsTimeBefore * LoggerConfig::SamplingFrequency;
	const unsigned int	after_packets = LoggerConfig::LimitsTimeBefore * LoggerConfig::SamplingFrequency;

	/** Over limit countdown. Decremented at each packet.
	 * 0 = no reading over limit.
	 * x = should write.
	 */
	unsigned int		overlimit_countdown = 0;

	tprintf("Entering write loop.\n");
	AccelerationSensors_RxQueue.Clear();
	Gps_RxQueue.Clear();
	for (;;) {
		unsigned int		packetcount_sensors = AccelerationSensors_RxQueue.Size();
		unsigned int		packetcount_gps = Gps_RxQueue.Size();
		LoggerIO::GPS		PacketGPS;
		LoggerIO::SENSORS	PacketSENSORS;
		LoggerIO::SENSORS	testpacket;
		SENSORS_RXBUFFER	PacketSENSORS_RXBUFFER;
		char				xbuf[100];

		// Print "Collecting..."
		while (packetcount_sensors < threshold) {
			sprintf(xbuf, "Collecting: %3d sec.", (threshold - packetcount_sensors) / LoggerConfig::SamplingFrequency);
			Display_MemoryCard(xbuf);
			Display_Process();
			packetcount_sensors = AccelerationSensors_RxQueue.Size();
			packetcount_gps = Gps_RxQueue.Size();
		}

		// Writing :)
		unsigned int	sensors_packets_written = 0;
		unsigned int	gps_packets_written = 0;
		for (unsigned int i=0; i<interval_packets; ++i) {
			const SENSORS_RXBUFFER&	testpacket_buffer = AccelerationSensors_RxQueue.Peek(before_packets);
			AccelerationSensors_RxQueue.Pop(PacketSENSORS_RXBUFFER);
			AccelerationSensors_Convert(testpacket, testpacket_buffer);
			AccelerationSensors_Convert(PacketSENSORS, PacketSENSORS_RXBUFFER);

			// 1. Display nice message :)
			if ((i % LoggerConfig::SamplingFrequency) == 0) {
				sprintf(xbuf, "Writing: %3d sec.", (interval_packets - i) / LoggerConfig::SamplingFrequency);
				Display_MemoryCard(xbuf);
				Display_Process();
			}

			// 2. Write all preceding GPS packets.
			while (!Gps_RxQueue.IsEmpty()) {
				const LoggerIO::GPS		gps_testpacket = Gps_RxQueue.Peek(0);
				if (gps_testpacket.Header.Tick < PacketSENSORS.Header.Tick) {
					Gps_RxQueue.Pop(PacketGPS);
					FixEndianGPS(PacketGPS);
					f.Write(&PacketGPS, sizeof(PacketGPS));
					++gps_packets_written;
				} else {
					break;
				}
			}

			// 3. Check the testpacket against limits.
			if (!AccelerationSensors_PacketWithinLimits(testpacket)) {
				overlimit_countdown = before_packets + after_packets;
			}

			// 4. Write, if needed :)
			if (overlimit_countdown > 0) {
				FixEndianSENSORS(PacketSENSORS);
				f.Write(&PacketSENSORS, sizeof(PacketSENSORS));
				--overlimit_countdown;
				++sensors_packets_written;
			}

		}
		tprintf("memorycard_loop: flush, wrote %d sensor packets, %d gps packets.\n",
			sensors_packets_written, gps_packets_written);
		f.Flush();
	}
}

//*******************************************************************
/** Initialize SPI interfaces for both LCD and SD/MMC.
 */
static void
spi_init()
{
	tprintf("spi...");
	static const gpio_map_t DIP204_SPI_GPIO_MAP =
	{
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION},  // Chip Select NPCS.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// add the spi options driver structure for the LCD DIP204
	spi_options_t spiOptions;
	spiOptions.reg          = DIP204_SPI_CS;
	spiOptions.baudrate     = 6*1000000;
	spiOptions.bits         = 8;
	spiOptions.spck_delay   = 0;
	spiOptions.trans_delay  = 0;
	spiOptions.stay_act     = 1;
	spiOptions.spi_mode     = 0;
	spiOptions.fdiv         = 0;
	spiOptions.modfdis      = 1;

	// Assign I/Os to SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));

	// Initialize as master
	spi_initMaster(DIP204_SPI, &spiOptions);

	// Set selection mode: variable_ps, pcs_decode, delay
	spi_selectionMode(DIP204_SPI, 0, 0, 0);

	// Enable SPI
	spi_enable(DIP204_SPI);

	// setup chip registers
	spi_setupChipReg(DIP204_SPI, &spiOptions, F_PBA);
	spiOptions.reg = 1;	// SD_MMC CS
	spi_setupChipReg(SD_MMC_SPI, &spiOptions, F_PBA);

	// Unselect the card and the display.
	spi_unselectChip(SD_MMC_SPI, 1);
	spi_unselectChip(DIP204_SPI, DIP204_SPI_CS);

	tprintf(" done.\n");
}


//*******************************************************************
static void
load_config()
{
	static bool	config_loaded = false;

	if (config_loaded) {
		return;
	}
	try {
		Filesystem::Blockdevice_SDMMC	sdmmc_card;
		Filesystem::FAT16				filesys(sdmmc_card);

		LoggerConfig::Load(filesys, "LOGGER.INI");
		config_loaded = true;
	} catch (const std::exception& e) {
		tprintf("Exception: %s\n", e.what());
		tprintf("Using defaults.\n");
	}
	LoggerConfig::PrintToDebug();
}

//*******************************************************************
int
main( void )
{
	LED_Display_Mask(LED0, LED0);	// LED0 - boot.

	// Start main clock.
	PLL0_Start();
	LED_Display_Mask(LED1, LED1);	// LED1 - PLL

	// Disable all interrupts.
	Disable_global_interrupt();
	// init the interrupts
	INTC_init_interrupts();
	// Enable all interrupts.
	Enable_global_interrupt();


	IUsart_Init(IUsart0, IUsart_RS232, INT0, 19200, NULL);
	tprintf("Boot.\n");

	LED_Display_Mask(LED2, LED2);	// LED2 - interrupts.

	spi_init();
	load_config();

	// Initialize modules.
	Display_Init();

	SDRAM_Init();


	AccelerationSensors_Init(LoggerConfig::SamplingFrequency);
	Gps_Init(LoggerConfig::GpsBaudRate);

	/** SDRAM distribution. */
	unsigned char*		sdram_ptr = reinterpret_cast<unsigned char*>(SDRAM);
	const unsigned int	max_time = 2 * (1 +
			LoggerConfig::WritingInterval +
			LoggerConfig::LimitsTimeBefore +
			LoggerConfig::LimitsTimeAfter);	// seconds
	tprintf("SDRAM: max time is %d seconds.\n", max_time);
	{
		// GPS rx queue buffer.
		const unsigned int	freq = 20;	// times per second.
		const unsigned int	nrof_items = max_time * freq;
		const unsigned int	nrof_bytes = nrof_items * sizeof(LoggerIO::GPS);
		Gps_RxQueue.SetBuffer(reinterpret_cast<LoggerIO::GPS*>(sdram_ptr), nrof_items);
		sdram_ptr += nrof_bytes;
	}
	{
		// Acceleration sensors rx queue buffer.
		const unsigned int	freq = LoggerConfig::SamplingFrequency;	// times per second.
		const unsigned int	nrof_items = max_time * freq;
		const unsigned int	nrof_bytes = nrof_items * sizeof(SENSORS_RXBUFFER);
		AccelerationSensors_RxQueue.SetBuffer(reinterpret_cast<SENSORS_RXBUFFER*>(sdram_ptr), nrof_items);
		sdram_ptr += nrof_bytes;
	}
	{
		const unsigned int	used_bytes = sdram_ptr - reinterpret_cast<unsigned char*>(SDRAM);
		const unsigned int	free_bytes = 32*1024*1024 - used_bytes;
		tprintf("SDRAM: %d bytes used, %d bytes free, theoretical maximum %d seconds.\n",
				used_bytes, free_bytes, max_time * ((free_bytes + used_bytes)/512) / ((used_bytes/512)));
	}

	/* Main loop. */
	tprintf("Entering main loop.\n");

	for (;;) {
		Display_Draw();
		try {
			memorycard_loop();
		} catch (const std::exception& e) {
			tprintf("Exception: %s\n", e.what());
			Display_Error(e.what());
		}
		Display_MemoryCard("Memory Card Lost.");
		Display_Sleep(5*1000);
	}
}

