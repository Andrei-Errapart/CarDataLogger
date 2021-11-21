/**
vim: ts=4
vim: shiftwidth=4
*/

/**
 *	FATLib.h:	interface for the FATLib.
 *	class FATLib:	a portable FAT decoder class which is hardware independent.
 *			All hardware specific operations are abstracted with the
 *			class HALayer.  The FATLib class operates with only the buffer
 *			which it passes to the class HALayer
 *
 *	Author:	Ivan Sham
 *	Date: JUly 1, 2004
 *	Version: 2.0
 *	Note: Developed for William Hue and Pete Rizun
 *
 *****************************************************************
 *  Change Log
 *----------------------------------------------------------------
 *  Date    |   Author          | Reason for change
 *----------------------------------------------------------------
 * Aug31/04     William Hue       Changed char types to
 *                                unsigned char.
 *                                Put more parentheses in abs()
 *                                macro.
 *
 * Jul18/04     Alex Jiang        Ported to FAB belt clip. Made
 *                                local variables and functions
 *                                static and moved to fatlib.c.
 *                                Prefixed public funtios with
 *                                "fat_".
 *
 * Jan02/05     William Hue       Various bugfixes and clean-up for
 *                                Circuit Cellar article.
 **/


#ifndef FATLib_h_
#define FATLib_h_

#include "HALayer.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILESYS_UNKNOWN	0
#define FILESYS_FAT16	1

#define CONTROL_DIRECTORY	true
#define CONTROL_FILE		false

#define BUFFER_SIZE 2

//#define abs(x)  (((x) > 0) ? (x) : (-(x)))

//------------------
// member functions:
//------------------

/**
 *	initialize the system
 *
 *	@return	0			UNKNOWN file system
 *	@return	1			FAT16 file system
 *	@return 3			could not set block length
 *	@return	4			could not initialize memory card
 *	@return 5			HALayer failure.
 **/
extern unsigned char fat_initialize(void);

/**
 *	closes the file indicated by the input
 *
 *	@param	fileHandle	handle of file to be closed
 *
 *	@return	0			file sucessfully closed
 *	@return	-1			invalid file handle
 *	@return	-2			invalid file system
 **/
extern signed char fat_close(signed char fileHandle);

/**
 *	opens the file indicated by the input path name.  If the pathname
 *	points to a valid file, the file is added to the list of currently
 *	opened files for reading and the unique file handle is returned.
 *
 *	@param	pathname	a pointer to the location of the file to be opened
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *
 *	@return	-1			invalid pathname
 *	@return	-2			file does not exist
 *	@return	-3			file already opened for writing
 *	@return -4			file already opened for reading
 *	@return -10			no handles available
 *	@return	-20			memory card error
 *	@return	-128		other error
 *	@return	...			file handle of sucessfully opened file
 **/
extern signed char fat_openRead(const char * pathname);

/**
 *	opens the file indicated by the input path name.  If the pathname
 *	points to a valid path, the file is created and added to the list of
 *	currently opened files for writing and the unique file handle is returned.
 *
 *	@param	pathname	a pointer to the location of the file to be opened
 *
 *	@return	-1			invalid pathname
 *	@return	-2			file already exist
 *	@return	-3			file already opened for writing
 *	@return	-4			no directory entries left
 *	@return -10			no handles available
 *	@return	-20			memory card error
 *	@return	-128		other error
 *	@return	(non-negative)	file handle of sucessfully opened file
 **/
extern signed char fat_openWrite(const char * pathname);

/**
 *	reads the content of the file identified by the input handle.  It reads from
 *	where the last read operation on the same file ended.  If it's the first time
 *	the file is being read, it starts from the beginning of the file.
 *
 *	@pre	nByte < SECTOR_SIZE
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	handle		handle of file to be read
 *	@param	nByte		number of bytes to read
 *
 *	@return	-10			memory card error
 *	@return -1			invalid handle
 *	@return -32768		other error
 *	@return	...			number of bytes read
 **/
extern signed int fat_read(signed char handle, unsigned char *buf, unsigned int nByte);

/**
 *	writes the content in the buffer to the file identified by the input handle.  It writes
 *	to where the last write operation on the same file ended.  If it's the first time
 *	the file is being written to, it starts from the beginning of the file.
 *
 *	@pre	nByte < SECTOR_SIZE
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	handle		handle of file to be written to
 *	@param	nByte		number of bytes to write
 *
 *	@return	-10			memory card error
 *	@return -1			invalid handle
 *	@return	-2			memory card is full
 *	@return -32768		other error
 *	@return	...			number of bytes written
 **/
extern signed int fat_write(signed char handle, unsigned char *buf, unsigned int nByte);

/** Size of currently open file.
 * \param[in]	handle	Currently opened file handle.
 * \return		Size of the given file.
 */
extern int
fat_filesize(
	const signed char handle
);

/**
 *	updates the file size in the directory table for all files with the update flag set
 **/
extern void fat_flush(void);

#if (0)
extern unsigned long get_first_sector(signed char handle);
#endif

#ifdef __cplusplus
}
#endif


#endif	// _FATLib_h_

