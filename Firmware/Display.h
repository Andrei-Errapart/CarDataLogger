/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Displah_h_
#define Display_h_

#include <stdbool.h>

/** Main thread: Initialize display. */
extern void Display_Init(void);
/** Main thread: Draw display. */
extern void Display_Draw(void);

/** Display memory card info line.
\param[in]	Memory card status.
 */
extern void Display_MemoryCard(
	const char*	line
);

/** Display GPS info.
\param[in]	line	GPS status line.
 */
extern void Display_Gps_Line(
	const char*	line
);

/** Display acceleration sensors line.
\param[in]	line	Text to be displayed on acceleration sensors line.
 */
extern void Display_AccelerationSensors(
	const char*	line
);

/** Display error message. */
extern void Display_Error(
	const char*	line
);

/** Process display updates... */
extern void
Display_Process(void);

/** Sleep for a given amount of time and refresh the display in the meantime :) */
extern void
Display_Sleep(
	const unsigned int	time_ms
);

#endif /* Display_h_ */

