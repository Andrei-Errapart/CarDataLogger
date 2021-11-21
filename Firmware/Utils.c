/**
vim: ts=4
vim: shiftwidth=4
*/

#include "project.h"
#include "Utils.h"
#include <stdbool.h>

//*******************************************************************
int
mystrncmp(
	const char* s1,
	const char* s2,
	const unsigned int n
)
{
	unsigned int	i;

	for (i=0; i<n; ++i) {
		const char	c1 = s1[i];
		const char	c2 = s2[i];
		if (c1==0 && c2==0) {
			return 0;
		}
		if (c1==0 || c1<c2) {
			return -1;
		}
		if (c2==0 || c1>c2) {
			return 1;
		}
	}

	return 0;
}

//*******************************************************************
void
mymemcpy(
	void* dst,
	const void* src,
	const unsigned int n
)
{
	char*			d = (char*)dst;
	const char*		s = (const char*)src;
	unsigned int	i;

	for (i=0; i<n; ++i) {
		d[i] = s[i];
	}
}

//*******************************************************************
void mystrcat(char* dst, const char* src)
{
	while (*dst != 0) {
		++dst;
	}

	while (*src != 0) {
		*dst = *src;
		++dst;
		++src;
	}
	*dst = 0;
}

//*******************************************************************
static void xtoa(unsigned long val, char *buf, unsigned radix, int negative)
{
	char *p;
	char *firstdig;
	char temp;
	unsigned digval;

	p = buf;

	if (negative) {
		// Negative, so output '-' and negate
		*p++ = '-';
		val = (unsigned long)(-(long) val);
	}

	// Save pointer to first digit
	firstdig = p;

	do {
		digval = (unsigned) (val % radix);
		val /= radix;

		// Convert to ascii and store
		if (digval > 9)
		*p++ = (char) (digval - 10 + 'a');
		else
		*p++ = (char) (digval + '0');
	} while (val > 0);

	// We now have the digit of the number in the buffer, but in reverse
	// order.  Thus we reverse them now.
	*p-- = '\0';
	do {
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;
		p--;
		firstdig++;
	} while (firstdig < p);
}

//*******************************************************************
char *itoa(const int val, char *buf, const int radix)
{
	if (radix == 10 && val < 0) {
		xtoa((unsigned long) val, buf, radix, 1);
	} else {
		xtoa((unsigned long)(unsigned int) val, buf, radix, 0);
	}

	return buf;
}

//*******************************************************************
static const char	xlat[16] = "0123456789ABCDEF";

//*******************************************************************
char*
hex_of_uint8(
	const unsigned char	x,
	char*				buf
)
{
	buf[0] = xlat[(x >> 4) & 0x0F];
	buf[1] = xlat[x & 0x0F];
	buf[2] = 0;
	return buf;
}

//*******************************************************************
uint8_t
_crc_ibutton_update(uint8_t crc, const uint8_t data)
{
	uint8_t i;

	crc = crc ^ data;
	for (i = 0; i < 8; i++) {
		crc = (crc >> 1) ^ ((crc & 0x01) * 0x8C);
	}
	return crc;
}

