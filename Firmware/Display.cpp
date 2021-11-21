/**
vim: ts=4
vim: shiftwidth=4
*/
#include "project.h"
#include "Display.h"
#include "AccelerationSensors.h"
#include "Gps.h"
#include "tprintf.h"

#define	ROW_LENGTH	20

static char MemoryCard_Line[ROW_LENGTH+1]	= { 0 };
static char Gps_Line[ROW_LENGTH+1]	= { 0 };
static char AccelerationSensors_Line[ROW_LENGTH+1]	= { 0 };
static char Error[ROW_LENGTH+1]			= { 0 };

void Display_Init(void)
{
	tprintf("Display...");
	dip204_init(backlight_PWM);
	tprintf(" done.\n");
}

//*******************************************************************
void Display_Draw(void)
{
  // Display default message.
  dip204_set_cursor_position(1,1);
  dip204_write_string(MemoryCard_Line[0]==0 ? "Memory Card N/A     " : MemoryCard_Line);

  dip204_set_cursor_position(1,2);
  dip204_write_string(Gps_Line[0]==0 ? "GPS N/A             " : Gps_Line);

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
void Display_Gps_Line(
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
void
Display_Process(void)
{
	Gps_Display_Process();
	AccelerationSensors_Display_Process();
	Display_Draw();
}

//*******************************************************************
extern void
Display_Sleep(
	const unsigned int	time_ms
)
{
	const unsigned long	ck = (unsigned long)time_ms * (F_CPU / 1000);
	// Use the CPU cycle counter (CPU and HSB clocks are the same).
	const unsigned long delay_start_cycle = Get_system_register(AVR32_COUNT);
	const unsigned long delay_end_cycle = delay_start_cycle + ck;

	// To be safer, the end of wait is based on an inequality test, so CPU cycle
	// counter wrap around is checked.
	if (delay_start_cycle <= delay_end_cycle) {
		while ((unsigned long)Get_system_register(AVR32_COUNT) < delay_end_cycle) {
			Display_Process();
		}
	} else {
		while ((unsigned long)Get_system_register(AVR32_COUNT) > delay_end_cycle) {
			Display_Process();
		}
	}
}

