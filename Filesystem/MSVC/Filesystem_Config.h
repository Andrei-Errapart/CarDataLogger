/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Config_h_
#define	Filesystem_Config_h_

#include <stdio.h>

#if defined(FILESYSTEM_DEBUG)
#define	filesystem_dprintf(args)	do { printf args; } while (0)
#else
#define	filesystem_dprintf(args)	do { } while (0)
#endif


#endif /* Filesystem_Config_h_ */
