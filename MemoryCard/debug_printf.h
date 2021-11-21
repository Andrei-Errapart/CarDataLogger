#ifndef debug_printf_h_
#define debug_printf_h_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void
debug_printf(
	const char*	fmt,
	...);

#ifdef __cplusplus
}
#endif


#endif /* debug_printf_h_ */

