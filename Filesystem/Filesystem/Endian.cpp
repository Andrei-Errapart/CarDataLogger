/**
vim: ts=4
vim: shiftwidth=4
*/
#include <Filesystem/Endian.h>

#if !defined(_MSC_VER)

namespace Filesystem {

//*******************************************************************
/** Swap LSB/MSB if on the big-endian machine. */
void FixEndian16(	uint16_t&	x)
{
	uint8_t*	ptr = reinterpret_cast<uint8_t*>(&x);
	const uint8_t	tmp = ptr[0];
	ptr[0] = ptr[1];
	ptr[1] = tmp;
}

//*******************************************************************
/** Swap LSB/MSB if on the big-endian machine. */
void FixEndian32(	uint32_t&	x)
{
	uint8_t*	ptr = reinterpret_cast<uint8_t*>(&x);
	const uint8_t	tmp0 = ptr[0];
	const uint8_t	tmp1 = ptr[1];
	ptr[0] = ptr[3];
	ptr[1] = ptr[2];
	ptr[2] = tmp1;
	ptr[3] = tmp0;
}

} // namespace Filesystem

#endif

