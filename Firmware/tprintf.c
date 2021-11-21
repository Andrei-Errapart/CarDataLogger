/**
vim: ts=4
vim: shiftwidth=4
*/
#include <stdio.h>
#include <stdarg.h>
#include "project.h"

#include "IClock.h"
#include "IUsart.h"
#include "tprintf.h"

static char	xbuf[512];

void
tprintf(
	const char*	fmt,
	...
)
{
	va_list		ap;
	int			i;
	int			r;
	va_start(ap, fmt);
	r = vsprintf(xbuf, fmt, ap);
	va_end(ap);

	if (r>0) {
		for (i=0; i<r; ++i) {
			if (xbuf[i]=='\n') {
				IUsart_Write(IUsart0, '\r');
			}
			IUsart_Write(IUsart0, xbuf[i]);
		}
	}
}

