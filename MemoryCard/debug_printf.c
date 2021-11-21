#include <stdarg.h>
#include <stdio.h>

#include "debug_printf.h"
#include "print_funcs.h"

static char	xbuf[500];

void
debug_printf(
	const char*	fmt,
	...)
{
	int	n;
	va_list	ap;

	va_start(ap, fmt);
	n = vsnprintf(xbuf, sizeof(xbuf), fmt, ap);
	va_end(ap);
	if (n>0 && n<sizeof(xbuf)) {
		xbuf[n] = 0;
		print_dbg(xbuf);
	}
}

