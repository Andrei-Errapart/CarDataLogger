/* vim: ts=2
 * vim: shiftwidth=2
 */
#ifndef stdint_h_
#define	stdint_h_

/** \file
MSVC++ version of "stdint.h".
*/

#ifdef _MSC_VER
	typedef __int8	int8_t;
	typedef __int16	int16_t;
	typedef __int32	int32_t;
	typedef __int64 int64_t;
	typedef unsigned __int8		uint8_t;
	typedef unsigned __int16	uint16_t;
	typedef unsigned __int32	uint32_t;
	typedef unsigned __int64	uint64_t;
#else
#error DO NOT INCLUDE IT WITH GCC.
#endif


#endif /* stdint_h_ */
