/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Utils_h_
#define Utils_h_

#include <stdint.h>		// u_int8_t, etc.
#include <stdbool.h>	// bool, true, false.

#if defined(__cplusplus)
extern "C" {
#endif

/** \file Utility functions. */

//*******************************************************************
/* ctype.h */
#define	isdigit(c)	((c)>='0' && (c)<='9')
#define isxdigit(c)	(((c)>='0' && (c)<='9') || ((c)>='a' && (c)<='f') || ((c)>='A' && (c)<='F'))
#define	islower(c)	((c)>='a' && (c)<='z')
#define	isupper(c)	((c)>='A' && (c)<='Z')

//*******************************************************************
/* string.h */
extern int mystrncmp(const char* s1, const char* s2, const unsigned int n);
extern void mymemcpy(void* dst, const void* src, const unsigned int n);
extern void mystrcat(char* dst, const char* src);

//*******************************************************************
/* stdlib.h */
extern char *itoa(const int val, char *buf, const int radix);

//*******************************************************************
/* Misc. */

/** Hexadecimal represenation of unsigned byte. Always two chars, uppercase. */
extern char*
hex_of_uint8(
	const unsigned char	x,
	char*				buf
);

/** Dallas/Maxim iButton CRC. */
extern uint8_t
_crc_ibutton_update(
	uint8_t			crc,
	const uint8_t	data
);


/** Is x between x1,x2 (including)? */
static bool inline
is_between(
	const unsigned int	x,
	const unsigned int	x1,
	const unsigned int	x2)
{
	return x2>x1 ? (x>=x1 && x<=x2) : (x>=x1 || x<=x2);
}

#if defined(__cplusplus)
}
#endif

#endif /* Utils_h_ */

