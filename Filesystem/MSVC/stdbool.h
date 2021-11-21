/* vim: ts=2
 * vim: shiftwidth=2
 */
#ifndef stdbool_h_
#define	stdbool_h_

/** \file
MSVC++ version of "stdbool.h".
*/

#if defined(_MSC_VER)
#if !defined(__cplusplus)
	typedef int	bool;
	enum {
		true = 1,
		false = 0
	};
#endif
#else
#error DO NOT INCLUDE IT WITH GCC.
#endif


#endif /* stdbool_h_ */

