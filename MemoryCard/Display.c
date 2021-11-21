/**
vim: ts=4
vim: shiftwidth=4
*/
#include "project.h"
#include "Display.h"

#define	ROW_LENGTH	20

static char MemoryCard_Line[ROW_LENGTH+1]	= { 0 };
static char Gps_Line[ROW_LENGTH+1]	= { 0 };
static char AccelerationSensors_Line[ROW_LENGTH+1]	= { 0 };
static char Error[ROW_LENGTH+1]			= { 0 };

void Display_Init(void)
{
	static const gpio_map_t DIP204_SPI_GPIO_MAP =
	{
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// add the spi options driver structure for the LCD DIP204
	spi_options_t spiOptions =
	{
		.reg          = DIP204_SPI_CS,
		.baudrate     = 12*1000000,
		.bits         = 8,
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.fdiv         = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));

	// Initialize as master
	spi_initMaster(DIP204_SPI, &spiOptions);

	// Set selection mode: variable_ps, pcs_decode, delay
	spi_selectionMode(DIP204_SPI, 0, 0, 0);

	// Enable SPI
	spi_enable(DIP204_SPI);

	// setup chip registers
	spi_setupChipReg(DIP204_SPI, &spiOptions, F_CPU);

	dip204_init(backlight_PWM);
}

//*******************************************************************
void Display_Draw(void)
{
  // Display default message.
  dip204_set_cursor_position(1,1);
  dip204_write_string(MemoryCard_Line[0]==0 ? "Memory Card N/A     " : MemoryCard_Line);

  dip204_set_cursor_position(1,2);
  dip204_write_string(Gps_Line[0]==0 ? "Gps N/A             " : Gps_Line);

  dip204_set_cursor_position(1,3);
  dip204_write_string(AccelerationSensors_Line[0]==0 ? "Sensors N/A         " : AccelerationSensors_Line);

  dip204_set_cursor_position(1,4);
  dip204_write_string(Error[0]==0 ? "System OK           " : Error);

  dip204_hide_cursor();
}

//*******************************************************************
static void
strncpy_row(
	char*				dst,
	const char*			src,
	const unsigned int	n
)
{
	unsigned int i;
	for (i=0; src[i]!=0 && i<n; ++i) {
		dst[i] = src[i];
	}
	for (; i<ROW_LENGTH; ++i) {
		dst[i] = ' ';
	}
	dst[ROW_LENGTH] = 0;
}

//*******************************************************************
void Display_MemoryCard(
	const char*	line
)
{
	strncpy_row(MemoryCard_Line, line, ROW_LENGTH);
}

//*******************************************************************
void Display_Gps(
	const char*	line
)
{
	strncpy_row(Gps_Line, line, ROW_LENGTH);
}

//*******************************************************************
void Display_AccelerationSensors(
	const char*	line
)
{
	strncpy_row(AccelerationSensors_Line, line, ROW_LENGTH);
}

//*******************************************************************
extern void Display_Error(
	const char*	line
)
{
	strncpy_row(Error, line, ROW_LENGTH);
}

//*******************************************************************
void Display_FatalError(
	const char*	line
)
{
	strncpy_row(Error, line, ROW_LENGTH);

	dip204_set_cursor_position(1,4);
	dip204_write_string(Error);
	dip204_hide_cursor();

	for (;;)
		;
}

