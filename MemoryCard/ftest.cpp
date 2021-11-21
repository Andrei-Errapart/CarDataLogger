/**
vim: ts=4
vim: shiftwidth=4
*/

#include <stdio.h>
#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"
#include "FATLib.h"
#include "IClock.h"
#include "Display.h"
#include "print_funcs.h"
#include "pmalloc.h"

#include <Filesystem/Blockdevice_SDMMC.h>
#include <Filesystem/FAT16.h>
#include <Filesystem/File.h>

//*******************************************************************
static void
print_file(
	const char*	filename
)
{
	signed char		handle = 0;
	int				filesize = 0;
	char			xbuf[512] = { 0 };
	unsigned char	filebuf[10+1] = { 0 };
	int				nread = -1;

	sprintf(xbuf, "print_file '%s'\n", filename);
	print_dbg(xbuf);

	handle = fat_openRead(filename);
	if (handle >= 0) {
		filesize = fat_filesize(handle);

		// Print file size.
		sprintf(xbuf, "File '%s' opened for reading, handle=%d, size=%d\n", filename, handle, filesize);
		print_dbg(xbuf);

		// Print file contents.
		nread = fat_read(handle, filebuf, sizeof(filebuf)-1);
		if (nread>0) {
			filebuf[nread] = 0;

			sprintf(xbuf, "Read %d bytes\n", nread);
			print_dbg(xbuf);

			print_dbg("File contents:");
			print_dbg((const char*)filebuf);
			print_dbg("\n");
		} else {
			sprintf(xbuf, "Failed to read, return value=%d\n", nread);
			print_dbg(xbuf);
		}
		fat_close(handle);
	} else {
		sprintf(xbuf, "File '%s' Not Found For Reading, error %d.\n", filename, handle);
		print_dbg(xbuf);
	}
}

//*******************************************************************
static void
write_file(
	const char*			filename,
	const char			c,
	const unsigned int	n
)
{
	char		xbuf[512];
	sprintf(xbuf, "write_file '%s'\n", filename);
	print_dbg(xbuf);

	// 1. Open the damn file :)
	const int handle = fat_openWrite(filename);

	if (handle >= 0) {
		// 2. Print file size.
		const int filesize = fat_filesize(handle);
		sprintf(xbuf, "File '%s' opened for writing, handle=%d, size=%d\n", filename, handle, filesize);
		print_dbg(xbuf);

		// 3. Set the buffer contents.
		unsigned char		writebuffer[512] = { 1 };
		const unsigned int	nreal = n<=sizeof(writebuffer) ? n : sizeof(writebuffer);
		for (unsigned int i=0; i<nreal; ++i) {
			writebuffer[i] = c;
		}

		// 4. Write to file.
		const int nwrite = fat_write(handle, writebuffer, nreal);
		if (nwrite>=0) {
			sprintf(xbuf, "Wrote %d bytes\n", nwrite);
			print_dbg(xbuf);
		} else {
			sprintf(xbuf, "Failed to read, return value=%d\n", nwrite);
			print_dbg(xbuf);
		}
		fat_close(handle);
	} else {
		sprintf(xbuf, "File '%s' Not Found For Writing, error %d.\n", filename, handle);
		print_dbg(xbuf);
	}
}

//*******************************************************************
static void
test_fatlib()
{
	char			fat_system = 0;

	fat_system = fat_initialize();

	switch (fat_system) {
	case 0:
		print_dbg("UNKNOWN File System.\n");
		break;
	case 1:
		print_dbg("FAT16 File System.\n");
		break;
	case 3:
		print_dbg("Error 3.\n");
		break;
	case 4:
		print_dbg("Error 4.\n");
		break;
	default:
		print_dbg("Unknown error.\n");
		break;
	}


	print_file("Logger.ini");
	write_file("Logger.bin", '1', 300);
	print_file("Logger.bin");

	for (;;) {
		Display_Draw();

		// delay, otherwise TeraTerm will screw up royally.
		delay_ms(5000);
	}
}

//*******************************************************************
static void
test_my_filesystem_stage2()
{
	try {
		Filesystem::Blockdevice_SDMMC	sdmmc_card;
		Filesystem::FAT16				filesys(sdmmc_card);

		const char*						filename = "LOGGER.BIN";

		Display_MemoryCard("Memory Card FOUND!");
		Display_Draw();

		for (;;) {
			// Writing.
			{
				Filesystem::File				f(filesys, filename, Filesystem::OPEN_CREATE);

				char				buffer[384];
				for (unsigned int i=0; i<7; ++i) {
					const char		c = '0' + (i % 10);
					memset(buffer, c, sizeof(buffer));
					f.Write(buffer, sizeof(buffer));
				}
			}

			for (unsigned int i=0; i<10; ++i) {
				delay_ms(1000);
			}

			// Reading.
			if (true) {
				Filesystem::File				f(filesys, filename, Filesystem::OPEN_READONLY);

				filesystem_dprintf(("File '%s' size is %d bytes.\n", filename, f.Size()));

				char	xbuf[1024] = { 0 };
				f.Read(xbuf, 1024);
				xbuf[10] = 0;
				filesystem_dprintf(("File contents: %s\n", xbuf));
			}

			for (unsigned int i=0; i<10; ++i) {
				delay_ms(1000);
			}
		}
	} catch (const std::exception& e) {
		Display_Error(e.what());
	}
}

//*******************************************************************
static void
test_my_filesystem(void)
{
	// 1. Initialize SPI.
	spi_options_t spiOptions;
	spiOptions.reg          = 1;
	spiOptions.baudrate     = 12*1000000;
	spiOptions.bits         = 8;
	spiOptions.spck_delay   = 0;
	spiOptions.trans_delay  = 0;
	spiOptions.stay_act     = 1;
	spiOptions.spi_mode     = 0;
	spiOptions.fdiv         = 0;
	spiOptions.modfdis      = 1;

	// Rest of the SPI is already initialized by DIP204 driver.
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP = {
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// Assign IO to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
			 sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Setup register according to spiOptions.
	spi_setupChipReg(SD_MMC_SPI, &spiOptions, F_PBA);

	for (;;) {
		Display_MemoryCard("No Memory Card.");
		Display_Draw();
		test_my_filesystem_stage2();
		Display_MemoryCard("Memory Card Lost.");
		Display_Draw();
		delay_ms(5000);
	}
}

//*******************************************************************
/*! \brief Main function. Executing starts here.
 */
int main(void)
{
	PLL0_Start();

	// Disable all interrupts.
	Disable_global_interrupt();
	// init the interrupts
	INTC_init_interrupts();
	// Enable all interrupts.
	Enable_global_interrupt();

	init_dbg_rs232(F_PBA);
	print_dbg("BOOT!\n");

	Display_Init();
	// pmalloc_init(PMALLOC_INIT_SKIPCHECK);
	delay_ms(10);


	for (;;) {
		test_my_filesystem();
		delay_ms(10*1000);
	}

	return 0;
}

