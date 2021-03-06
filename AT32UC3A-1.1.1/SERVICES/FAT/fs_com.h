/* This header file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FAT 12/16/32 Services.
 *
 * This file provides a set of definitions for FAT module interface.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/* Copyright (c) 2007, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FAT_COM_H_
#define _FAT_COM_H_

#include "conf_explorer.h"

#undef _GLOBEXT_
#if (defined _fat_c_)
#  define _GLOBEXT_
#else
#  define _GLOBEXT_ extern
#endif


//------- Test the configuration in conf_explorer.h

#ifndef  FS_FAT_12
#  error FS_FAT_12 must be defined with ENABLED or DISABLED in conf_explorer.h
#endif
#ifndef  FS_FAT_16
#  error FS_FAT_16 must be defined with ENABLED or DISABLED in conf_explorer.h
#endif
#ifndef  FS_FAT_32
#  error FS_FAT_32 must be defined with ENABLED or DISABLED in conf_explorer.h
#endif

#ifndef  FS_MULTI_PARTITION
#  error FS_MULTI_PARTITION must be defined with ENABLED or DISABLED in conf_explorer.h
#endif
#ifndef  FS_NB_NAVIGATOR
#  error FS_NB_NAVIGATOR must be defined in conf_explorer.h
#endif


//_____ M A C R O S ________________________________________________________

//_____ D E F I N I T I O N S ______________________________________________


//**** VALUE of communication

//! \name Mode for file control name fonctions
//! @{
#define  FS_NAME_GET          TRUE
#define  FS_NAME_CHECK        FALSE
//! @}

//! \name Mode for disk serial number fonction
//! @{
#define  FS_SN_READ           TRUE
#define  FS_SN_WRITE          FALSE
//! @}

//! \name Mode for disk label fonction
//! @{
#define  FS_LABEL_READ        TRUE
#define  FS_LABEL_WRITE       FALSE
//! @}

//! \name Status returned by fonction copy file
//! @{
#define  COPY_BUSY            0x00
#define  COPY_FAIL            0x01
#define  COPY_FINISH          0x02
//! @}

//! \name Type of date for get date fonction
//! @{
#define  FS_DATE_CREATION     TRUE
#define  FS_DATE_LAST_WRITE   FALSE
//! @}

//! \name Type of file entry
//! @{
#define  FS_FILE              TRUE
#define  FS_DIR               FALSE
//! @}

//! \name Directory ID (it is the first cluster of directory list)
//! @{
#define  FS_ID_DIR_EMPTY      0xFFFFFFFF     // ID no directory = bad cluster
#define  FS_ID_DIR_ROOT       0              // ID of the root directory is always the cluster 0
//! @}

//! \name File entry attributes
//! @{
#define  FS_ATTR_FILE         ((Byte)0x00)   // Normal File Attribute
#define  FS_ATTR_READ_ONLY    ((Byte)0x01)   // Read Only File Attribute
#define  FS_ATTR_HIDDEN       ((Byte)0x02)   // Hidden File Attribute
#define  FS_ATTR_SYSTEM       ((Byte)0x04)   // System File Attribute
#define  FS_ATTR_VOLUME_ID    ((Byte)0x08)   // Volume id Attribute
#define  FS_ATTR_DIRECTORY    ((Byte)0x10)   // Directory Attribute
#define  FS_ATTR_ARCHIVE      ((Byte)0x20)   // Archive Attribute
#define  FS_ATTR_LFN_ENTRY    ((Byte)0x0F)   // LFN entry attribute
#define  Fat_is_not_a_file    ((FS_ATTR_DIRECTORY & fs_g_nav_entry.u8_attr) || (FS_ATTR_VOLUME_ID & fs_g_nav_entry.u8_attr))
//! @}

//! \name File open mode
//! @{
#define  FOPEN_READ_ACCESS    0x01                                                                    // autorize the read access
#define  FOPEN_WRITE_ACCESS   0x02                                                                    // autorize the write access
#define  FOPEN_CLEAR_SIZE     0x04                                                                    // reset size
#define  FOPEN_CLEAR_PTR      0x08                                                                    // reset flux pointer
#define  FOPEN_MODE_R         (FOPEN_READ_ACCESS|FOPEN_CLEAR_PTR)                                     //!< R   access, flux pointer = 0, size not modify
#define  FOPEN_MODE_R_PLUS    (FOPEN_READ_ACCESS|FOPEN_WRITE_ACCESS|FOPEN_CLEAR_PTR)                  //!< R/W access, flux pointer = 0, size not modify
#define  FOPEN_MODE_W         (FOPEN_WRITE_ACCESS|FOPEN_CLEAR_PTR|FOPEN_CLEAR_SIZE)                   //!< W   access, flux pointer = 0, size = 0
#define  FOPEN_MODE_W_PLUS    (FOPEN_READ_ACCESS|FOPEN_WRITE_ACCESS|FOPEN_CLEAR_PTR|FOPEN_CLEAR_SIZE) //!< R/W access, flux pointer = 0, size = 0
#define  FOPEN_MODE_APPEND    (FOPEN_WRITE_ACCESS)                                                    //!< W   access, flux pointer = at the end, size not modify
//! @}

//! Flag end of file (see file_getc fonction)
#define  FS_EOF               0xFFFF

//! \name Options of format fonction
//! @{
#define  FS_FORMAT_DEFAULT       0x01     //!< The format routine choose the better FAT for the device
#define  FS_FORMAT_FAT           0x02     //!< Force FAT12 or FAT16 format
#define  FS_FORMAT_FAT32         0x03     //!< Force FAT32 format
#define  FS_FORMAT_NOMBR_FLAG    0x80     //!< MBR is mandatory for USB device on MacOS, and no MBR is mandatory for CD-ROM USB device on Windows
#define  FS_FORMAT_DEFAULT_NOMBR (FS_FORMAT_NOMBR_FLAG | FS_FORMAT_DEFAULT)  
#define  FS_FORMAT_FAT_NOMBR     (FS_FORMAT_NOMBR_FLAG | FS_FORMAT_FAT)
#define  FS_FORMAT_FAT32_NOMBR   (FS_FORMAT_NOMBR_FLAG | FS_FORMAT_FAT32)
//! @}


//**** STRUCTURES of communication

//! \name Type definition for UNICODE support
//! @{
typedef U16 UNICODE;
typedef U8        _MEM_TYPE_SLOW_ * FS_STRING;
typedef UNICODE   _MEM_TYPE_SLOW_ * FS_STR_UNICODE;
//! @}

//! Struture to store a pointer on a file
typedef struct {
   U8    u8_lun;                       //!< number of logical driver
#if (FS_MULTI_PARTITION == ENABLED)
   U8    u8_partition;                 //!< number of partition - 1 (0 or 1) (if FS_MULTI_PARTITION == ENABLED)
#endif
   U32   u32_cluster_sel_dir;          //!< number of first cluster of directory selected (0 for the root directory)
   U16   u16_entry_pos_sel_file;       //!< entry offset of file selected in directory selected (unit = FS_SIZE_FILE_ENTRY)
} Fs_index;


//**** ERROR CODE of communication

//! \name File System errors list
//! @{
#define  FS_ERR_HW               FAIL+0   //!< Hardware driver error
#define  FS_ERR_NO_FORMAT        FAIL+1   //!< The selected drive isn't formated
#define  FS_ERR_NO_PART          FAIL+2   //!< The partition selected doesn't exist
#define  FS_ERR_NO_SUPPORT_PART  FAIL+3   //!< The partition selected isn't supported by FS
#define  FS_ERR_TOO_FILE_OPEN    FAIL+4   //!< The navigation have already open a file
#define  FS_ERR_END_OF_DRIVE     FAIL+5   //!< There are not other drivers
#define  FS_ERR_BAD_POS          FAIL+6   //!< The position is over the file
#define  FS_ERR_FS               FAIL+7   //!< File system error
#define  FS_ERR_NO_FIND          FAIL+8   //!< File no found
#define  FS_ERR_ENTRY_EMPTY      FAIL+9   //!< File entry empty
#define  FS_ERR_ENTRY_BAD        FAIL+10  //!< File entry bad
#define  FS_ERR_ENTRY_BADTYPE    FAIL+11  //!< File entry isn't a good type
#define  FS_ERR_NO_DIR           FAIL+12  //!< The current file isn't a directory
#define  FS_ERR_NO_MOUNT         FAIL+13  //!< The partition isn't mounted
#define  FS_ERR_NO_FILE_SEL      FAIL+14  //!< There are no file selected
#define  FS_NO_LAST_LFN_ENTRY    FAIL+15  //!< The entry file isn't the last long file entry
#define  FS_ERR_NAVDST_FILE_OPEN FAIL+16  //!< The navigation destination have a file openned
#define  FS_ERR_ID_FILE          FAIL+17  //!< The file identify is bad
#define  FS_ERR_NO_FILE          FAIL+18  //!< The entry selected isn't a file
#define  FS_LUN_WP               FAIL+19  //!< Drive is read only
#define  FS_ERR_READ_ONLY        FAIL+20  //!< File is read access only
#define  FS_ERR_NAME_INCORRECT   FAIL+21  //!< The name don't corresponding at filter name
#define  FS_ERR_FILE_NO_OPEN     FAIL+22  //!< No file is open
#define  FS_ERR_HW_NO_PRESENT    FAIL+23  //!< Device not present
#define  FS_ERR_IS_ROOT          FAIL+24  //!< There aren't parent because the current directory is a root directory
#define  FS_ERR_OUT_LIST         FAIL+25  //!< The position is at out off the cluster list
#define  FS_ERR_NO_FREE_SPACE    FAIL+26  //!< No free cluster found in FAT
#define  FS_ERR_INCORRECT_NAME   FAIL+27  //!< Incorrect name, this must be not containt char \/:*?"<>|
#define  FS_ERR_DIR_NOT_EMPTY    FAIL+28  //!< This fonction erase only the file and the empty directory
#define  FS_ERR_WRITE_ONLY       FAIL+29  //!< File is write access only
#define  FS_ERR_MODE_NOAVIALABLE FAIL+30  //!< This fpoen mode isn't avialable
#define  FS_ERR_EOF              FAIL+31  //!< End of file
#define  FS_ERR_BAD_SIZE_FAT     FAIL+32  //!< The disk size is not supported by file system selected
#define  FS_ERR_COMMAND          FAIL+33  //!< Bad command send to explorer task
#define  FS_ERR_BUFFER_FULL      FAIL+34  //!< Buffer is too small
#define  FS_ERR_COPY_DIR         FAIL+35  //!< Impossible to copy a directory
#define  FS_ERR_COPY_RUNNING     FAIL+36  //!< A copy action is always running
#define  FS_ERR_COPY_IMPOSSIBLE  FAIL+37  //!< A copy is impossible
#define  FS_ERR_BAD_NAV          FAIL+38  //!< The navigator doesn't exist
#define  FS_ERR_FILE_OPEN        FAIL+39  //!< The file is already open
#define  FS_ERR_FILE_OPEN_WR     FAIL+40  //!< The file is already open in write mode
#define  FS_ERR_FILE_EXIST       FAIL+41  //!< The file is exist
#define  FS_ERR_POS_UNICODE_BAD  FAIL+42  //!< A read ASCII can not mixed with a read unicode

#define  FS_ERR_PL_NOT_OPEN      FAIL+50  //!< The play list isn't open
#define  FS_ERR_PL_ALREADY_OPEN  FAIL+51  //!< The play list is already open
#define  FS_ERR_PL_LST_END       FAIL+52  //!< You are at the end of play list
#define  FS_ERR_PL_LST_BEG       FAIL+53  //!< You are at the beginning of play list
#define  FS_ERR_PL_OUT_LST       FAIL+54  //!< You are outside of the play list
#define  FS_ERR_PL_READ_ONLY     FAIL+55  //!< Impossible to modify play list
//! @}


//! \name Compilation feature levels
//! @{
#define  FSFEATURE_READ                (0x00)   //!< All read fonctions
#define  FSFEATURE_WRITE               (0x02)   //!< nav_file_copy(), nav_file_paste(), nav_file_del(), file_create(), file_open(MODE_WRITE), file_write(), file_putc()
#define  FSFEATURE_WRITE_COMPLET       (0x06)   //!< FSFEATURE_WRITE + nav_drive_format(), nav_dir_make(), nav_file_rename(), nav_file_dateset(), nav_file_attributset()
#define  FSFEATURE_ALL                 (0xFF)   //!< All file system module
//! @}


//! Type of file system status
typedef  U8                   Fs_status;
//! Global file system status used to return error
_GLOBEXT_ _MEM_TYPE_SLOW_ Fs_status fs_g_status;

#endif  // _FAT_COM_H_
