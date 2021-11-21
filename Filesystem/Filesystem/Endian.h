/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Endian_h_
#define	Filesystem_Endian_h_

#include <stdint.h>

namespace Filesystem {

#if defined(_MSC_VER)
#	define	FixEndian16(x)	do { } while (0)
#	define	FixEndian32(x)	do { } while (0)
#else
/** Swap LSB/MSB if on the big-endian machine. */
void FixEndian16(	uint16_t&	x);
/** Swap LSB/MSB if on the big-endian machine. */
void FixEndian32(	uint32_t&	x);
#endif

} // namespace Filesystem


#endif
