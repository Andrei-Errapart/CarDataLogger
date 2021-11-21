/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Displah_h_
#define Display_h_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Main thread: Initialize display. */
extern void Display_Init(void);
/** Main thread: Draw display. */
extern void Display_Draw(void);

/** Display memory card info line. */
extern void Display_MemoryCard(
	const char*	line
);

/** Display GPS info line.
\param[in]	line	NMEA line, without terminating CRLF.
 */
extern void Display_Gps(
	const char*	line
);

/** Display acceleration sensors info line.
\param[in]	line	Text to be displayed on acceleration sensors line.
 */
extern void Display_AccelerationSensors(
	const char*	line
);

/** Display error message. */
extern void Display_Error(
	const char*	line
);

/** Display fatal error and hang. */
extern void Display_FatalError(
	const char*	line
);

#ifdef __cplusplus
}
#endif

#endif /* Display_h_ */

