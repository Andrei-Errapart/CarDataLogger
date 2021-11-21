/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef tprintf_h_
#define	tprintf_h_


#include "project.h"

#if defined(_MSC_VER)
#	include <stdio.h>
#	define tprintf	printf
#else

#if defined(__cplusplus)
extern "C" {
#endif
/** Trace output. */
extern void tprintf(const char* fmt, ...);
#if defined(__cplusplus)
}
#endif

#endif

#endif /* tprintf_h_ */

