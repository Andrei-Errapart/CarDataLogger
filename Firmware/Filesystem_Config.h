/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Config_h_
#define	Filesystem_Config_h_

#include <stdio.h>

#include "project.h"

#if defined(FILESYSTEM_DEBUG)
#	include "IClock.h"
#	include "tprintf.h"
#	define	filesystem_dprintf(args)	do { tprintf args; } while (0)
#else
#	define	filesystem_dprintf(args)	do { } while (0)
#endif

#define	FILESYSTEM_SDMMC_SPI_SELECT()		spi_selectChip(SD_MMC_SPI, 1)
#define	FILESYSTEM_SDMMC_SPI_UNSELECT()		spi_unselectChip(SD_MMC_SPI, 1)
#define	FILESYSTEM_SDMMC_SPI_READ(dataptr)	spi_read(SD_MMC_SPI, dataptr)
#define	FILESYSTEM_SDMMC_SPI_WRITE(data)	spi_write(SD_MMC_SPI, data)


#endif /* Filesystem_Config_h_ */
