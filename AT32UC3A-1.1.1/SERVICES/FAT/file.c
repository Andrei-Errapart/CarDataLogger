/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FAT 12/16/32 Services.
 *
 * This file defines a useful set of functions for the file accesses on
 * AVR32 devices.
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


//_____  I N C L U D E S ___________________________________________________
#include "conf_explorer.h"
#include "file.h"
#include "navigation.h"
#include LIB_MEM
#include LIB_CTRLACCESS


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________

//! Use the "FAT sector cache" to store a file sector during a file_putc() or file_getc() routines.
extern   _MEM_TYPE_SLOW_   U8    fs_g_sector[ FS_SIZE_OF_SECTOR ];

//! Internal flag to signal the string format (ASCII, UNICODE) to use for getc() and putc() routines.
Bool     g_b_file_unicode  =  FALSE;

//_____ D E C L A R A T I O N S ____________________________________________

// Sub routine to store the internal segment structure to other structure
static   void  file_load_segment_value( Fs_file_segment _MEM_TYPE_SLOW_ *segment );


//**********************************************************************
//********** String format select for gets, getc, puts, putc ***********

//! This fonction select the UNICODE mode for the routines getc() and putc()
//!
void  file_string_unicode( void )
{
   g_b_file_unicode = TRUE;
}

//! This fonction select the ASCII mode for the routines getc() and putc()
//!
void  file_string_ascii( void )
{
   g_b_file_unicode = FALSE;
}


//! This fonction check if a file is selected and it is not a directory
//!
//! @return    TRUE, a file is selected
//! @return    FALSE, otherwise
//!
Bool  file_ispresent( void )
{
   if( !fat_check_mount_select() )
      return FALSE;
   return fat_check_is_file();
}


//! This fonction open the file selected
//!
//! @param     fopen_mode  option to open file : <br>
//!                        FOPEN_MODE_R         R   access, flux pointer = 0, size not modify <br>
//!                        FOPEN_MODE_R_PLUS    R/W access, flux pointer = 0, size not modify <br>
//!                        FOPEN_MODE_W         W   access, flux pointer = 0, size = 0 <br>
//!                        FOPEN_MODE_W_PLUS    R/W access, flux pointer = 0, size = 0 <br>
//!                        FOPEN_MODE_APPEND    W   access, flux pointer = at the end, size not modify <br>
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  file_open( U8 fopen_mode )
{
   if( !fat_check_mount_select_noopen())
      return FALSE;

   if( !fat_check_is_file())
      return FALSE;

   if(FOPEN_WRITE_ACCESS & fopen_mode)
   {
      if( !fat_check_nav_access_file( TRUE ) )
         return FALSE;
#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
      if (FS_ATTR_READ_ONLY & fs_g_nav_entry.u8_attr)
      {
         fs_g_status = FS_ERR_READ_ONLY;  // File is read only
         return FALSE;
      }
      if( mem_wr_protect( fs_g_nav.u8_lun  ))
      {
         fs_g_status = FS_LUN_WP;  // Disk read only
         return FALSE;
      }
#else
      fs_g_status = FS_ERR_MODE_NOAVIALABLE;
      return FALSE;
#endif  // FS_LEVEL_FEATURES
   }
   else
   {
      if( !fat_check_nav_access_file( FALSE ) )
         return FALSE;
   }

   if(FOPEN_CLEAR_SIZE & fopen_mode)
   {
      fs_g_nav_entry.u32_size    = 0;     // The size is null
   }
   if(FOPEN_CLEAR_PTR & fopen_mode)
   {
      fs_g_nav_entry.u32_pos_in_file = 0; 
   }
   else
   {  // Go to at the end of file
      fs_g_nav_entry.u32_pos_in_file = fs_g_nav_entry.u32_size;
   }
   fs_g_nav_entry.u8_open_mode = fopen_mode;
   return TRUE;
}


//! This fonction load the global segment in param segment
//!
//! @param     segment  Pointer on the segment structure to load
//!
static void file_load_segment_value( Fs_file_segment _MEM_TYPE_SLOW_ *segment )
{
   segment->u8_lun = fs_g_nav.u8_lun;
   segment->u32_addr = fs_g_seg.u32_addr;
   segment->u32_size_or_pos = fs_g_seg.u32_size_or_pos;
}


//! This fonction return a continue physical memory segment corresponding at a file to read
//!
//! @param     segment  Pointer on the segment structure: <br>
//!                     ->u32_size_or_pos    IN,   shall containt the maximum number of sector to read in file (0 = unlimited) <br>
//!                     ->u32_size_or_pos    OUT,  containt the segment size (unit sector) <br>
//!                     ->other              IN,   ignored <br>
//!                     ->other              OUT,  containt the segment position <br>
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! This routine is interesting to read a file via a DMA and avoid the file system decode
//! because this routine return a physical memory segment without File System data.
//! Note: the file can be fragmented and it can be necessary to call several time file_read().
//! @endverbatim
//!
Bool  file_read( Fs_file_segment _MEM_TYPE_SLOW_ *segment )
{
   U8 u8_nb_sector_truncated;

   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_READ_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_WRITE_ONLY;
      return FALSE;
   }
   
   if ( file_eof() )
   {
      // End of the file
      fs_g_status = FS_ERR_EOF;
      return FALSE;
   }

   if( !fat_read_file(FS_CLUST_ACT_SEG))
   {
      if( FS_ERR_OUT_LIST == fs_g_status )
         fs_g_status = FS_ERR_EOF;  // translate the error
      return FALSE;
   }
   // Truncate the segment found if more larger than size asked
   if( (segment->u32_size_or_pos != 0)  // if not undefine limit
   &&  (segment->u32_size_or_pos < fs_g_seg.u32_size_or_pos) )
   {
      u8_nb_sector_truncated   = fs_g_seg.u32_size_or_pos - segment->u32_size_or_pos;
      fs_g_seg.u32_size_or_pos = segment->u32_size_or_pos ;
   }else{
      u8_nb_sector_truncated = 0;
   }

   // Update position file
   fs_g_nav_entry.u32_pos_in_file += (U32)fs_g_seg.u32_size_or_pos * FS_SIZE_OF_SECTOR;
   if( fs_g_nav_entry.u32_size < fs_g_nav_entry.u32_pos_in_file )
   {  
      // Limit the read
      // FYC: this case is posible, if the end of file isn't at the end of cluster
      // compute the sector not used by file      
      U8 u8_nb_sector_not_used;
      
      // Compute the number of sector used in last cluster
      // remark: also the two first byte of size is used, because the cluster size can't be more larger
      u8_nb_sector_not_used = LSB1( fs_g_nav_entry.u32_size ) >> (FS_SHIFT_B_TO_SECTOR-8);
      if( 0 != (fs_g_nav_entry.u32_size & FS_MASK_SIZE_OF_SECTOR) )
      {  // last sector of file isn't full, but it must be read
         u8_nb_sector_not_used++;
      }

      // Compute the number of sector not used in last cluster
      u8_nb_sector_not_used = fs_g_nav.u8_BPB_SecPerClus - (u8_nb_sector_not_used % fs_g_nav.u8_BPB_SecPerClus);
      // if all space of cluster isn't used, then it is wrong
      if( u8_nb_sector_not_used == fs_g_nav.u8_BPB_SecPerClus )
         u8_nb_sector_not_used = 0; // The file use all last cluster

      // subtract this value a the position and segment
      u8_nb_sector_not_used -= u8_nb_sector_truncated;
      fs_g_seg.u32_size_or_pos -= u8_nb_sector_not_used; // Unit sector
      fs_g_nav_entry.u32_pos_in_file -= ((U16)u8_nb_sector_not_used) << FS_SHIFT_B_TO_SECTOR;   // unit byte
   }
   file_load_segment_value( segment );
   return TRUE;
}


//! This fonction copy in data buffer the data file corresponding at the current position
//!
//! @param     buffer         buffer to fill
//! @param     u16_buf_size   buffer size
//!
//! @return    number of byte read
//! @return    0, in case of error
//!
U16   file_read_buf( U8 _MEM_TYPE_SLOW_ *buffer , U16 u16_buf_size )
{
   _MEM_TYPE_FAST_ U16 u16_nb_read_tmp;
   _MEM_TYPE_FAST_ U16 u16_nb_read;
   _MEM_TYPE_FAST_ U16 u16_pos_in_sector;
   _MEM_TYPE_FAST_ U32 u32_byte_remaining;

   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_READ_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_WRITE_ONLY;
      return FALSE;
   }
   
   u16_nb_read = 0;  // Init to "no data read"

   while( 0 != u16_buf_size )
   {
      if ( file_eof() )
      {
         // End of the file
         fs_g_status = FS_ERR_EOF;
         return u16_nb_read;
      }
      u32_byte_remaining = fs_g_nav_entry.u32_size-fs_g_nav_entry.u32_pos_in_file;
      u16_pos_in_sector = fs_g_nav_entry.u32_pos_in_file % FS_SIZE_OF_SECTOR;

      if( (0== u16_pos_in_sector)
      &&  (FS_SIZE_OF_SECTOR <= u32_byte_remaining)
      &&  (FS_SIZE_OF_SECTOR <= u16_buf_size))
      {
         if( u16_buf_size <= u32_byte_remaining)
         {
            u16_nb_read_tmp = u16_buf_size;
         }else{
            u16_nb_read_tmp = u32_byte_remaining;
         }
         u16_nb_read_tmp = u16_nb_read_tmp / FS_SIZE_OF_SECTOR;  // read a modulo sector size

         // It is possible to read more than sector
         if( !fat_read_file(FS_CLUST_ACT_SEG))
         {
            if( FS_ERR_OUT_LIST == fs_g_status )
               fs_g_status = FS_ERR_EOF;  // translate the error
            return u16_nb_read;
         }
         // Truncate the segment found if more larger than size asked
         if( u16_nb_read_tmp > fs_g_seg.u32_size_or_pos )
         {
            u16_nb_read_tmp = fs_g_seg.u32_size_or_pos;
         }else{
            fs_g_seg.u32_size_or_pos = u16_nb_read_tmp;
         }
         
         // Fill buffer directly
         while( 0 != fs_g_seg.u32_size_or_pos )
         {
            if( CTRL_GOOD != memory_2_ram( fs_g_nav.u8_lun  , fs_g_seg.u32_addr, buffer))
            {
               fs_g_status = FS_ERR_HW;
               return u16_nb_read;
            }
            fs_g_seg.u32_size_or_pos--;
            fs_g_seg.u32_addr++;
            buffer += FS_SIZE_OF_SECTOR;
         }
         // Translate to Byte unit
         u16_nb_read_tmp *= FS_SIZE_OF_SECTOR;
      }
      else
      {
         // Get current file sector in internal FS buffer
         if( !fat_read_file( FS_CLUST_ACT_ONE ))
         {
            if( FS_ERR_OUT_LIST == fs_g_status )
            {  // Translate the error
               fs_g_status = FS_ERR_EOF;   // End of file
            }
            return u16_nb_read;
         }
         
         // Compute the number of data to transfer
         u16_nb_read_tmp = FS_SIZE_OF_SECTOR - u16_pos_in_sector;  // Compute the maximum to next sector limit
         if( u16_nb_read_tmp > u32_byte_remaining )
            u16_nb_read_tmp = u32_byte_remaining;
         if( u16_nb_read_tmp > u16_buf_size )
            u16_nb_read_tmp = u16_buf_size;

         // Fill buffer
         memcpy_ram2ram( buffer , &fs_g_sector[ u16_pos_in_sector ], u16_nb_read_tmp );
         buffer += u16_nb_read_tmp;
      }
      // Update positions
      fs_g_nav_entry.u32_pos_in_file   += u16_nb_read_tmp;
      u16_nb_read                      += u16_nb_read_tmp;
      u16_buf_size                     -= u16_nb_read_tmp;
   }
   return u16_nb_read;  // Buffer is full
}


//! This fonction copy in a data buffer the current line of open file
//!
//! @param     string         string to fill
//! @param     u16_str_size   string size (unit ASCII or UNICODE)
//!
//! @return    number of byte read
//! @return    0, in case of error
//!
//! @verbatim
//! Before, if you want get a UNICODE string then call file_string_unicode().
//! This routine write in string the back line character.
//! @endverbatim
//!
U16   file_gets( U8 _MEM_TYPE_SLOW_ *string , U16 u16_str_size )
{
   U16 u16_save_buf_size;
   U16 u16_nb_read;

   if( g_b_file_unicode )
   {
      u16_nb_read = file_read_buf( string , u16_str_size*2 );
      u16_nb_read /= 2;
   }
   else
   {
      u16_nb_read = file_read_buf( string , u16_str_size );
   }
   if( 0 == u16_nb_read )
      return 0;

   u16_save_buf_size = u16_str_size;

   // Search end of line
   while( 1 )
   {                      
      // Increment the number of character read
      u16_nb_read--;
      u16_str_size--;

      if( (( g_b_file_unicode) && ('\0'  == ((FS_STR_UNICODE)string )[0]))
      ||  ((!g_b_file_unicode) && ('\0'  == string [0])) )
      {
         // End of texte file
         break;
      }
      
      if( 0 == u16_str_size )
      {
         // string full (u16_str_size = u16_nb_read = 0)
         u16_nb_read = u16_str_size = 1;              // Decrement the number of character read
         break;
      }

      if( (( g_b_file_unicode) && ('\n'  == ((FS_STR_UNICODE)string )[0]))
      ||  ((!g_b_file_unicode) && ('\n'  == string [0]))
      ||  ( 0 == u16_nb_read ) )
      {  
         // End of line
         string += (g_b_file_unicode? 2 : 1 );  // Go to next character
         break;
      }
      string += (g_b_file_unicode? 2 : 1 );  // Go to next character
   }
   // add a '\0' at the end of string
   if( g_b_file_unicode )
   {
      ((FS_STR_UNICODE)string )[0] = '\0';
      file_seek( u16_nb_read*2 , FS_SEEK_CUR_RE );
   }else{
      string [0] = '\0';
      file_seek( u16_nb_read , FS_SEEK_CUR_RE );
   }
   return (u16_save_buf_size-u16_str_size) ;
}


//! This fonction return the next character at current file position
//!
//! @return    The character reading (UNICOCE, or ASCII storage in LSB)
//! @return    EOF, in case of error or end of file
//!
//! @verbatim
//! Before, if you want get a UNICODE character then call file_string_unicode().
//! This routine return ALL characters.
//! @endverbatim
//!
U16   file_getc( void )
{
   U16   u16_byte;
   U16   u16_buf_pos;

   while(1)
   {
      if( !fat_check_mount_select_open())
         break;
      if(!(FOPEN_READ_ACCESS & fs_g_nav_entry.u8_open_mode))
      {
         fs_g_status = FS_ERR_WRITE_ONLY;
         break;
      }
      if ( file_eof() )
      {
         // End of the file
         fs_g_status = FS_ERR_EOF;
         break;
      }
  
      if( !fat_read_file( FS_CLUST_ACT_ONE ))
      {
         if( FS_ERR_OUT_LIST == fs_g_status )
         {  // Translate the error
            fs_g_status = FS_ERR_EOF;   // End of file
         }
         break;
      }
      
      u16_buf_pos = fs_g_nav_entry.u32_pos_in_file & FS_MASK_SIZE_OF_SECTOR;

      if( g_b_file_unicode )
      {
         if( 0x01 & u16_buf_pos )
         {
            fs_g_status = FS_ERR_POS_UNICODE_BAD;   // Error in position file
            break;
         }
         MSB(u16_byte) = fs_g_sector[ u16_buf_pos ];
         LSB(u16_byte) = fs_g_sector[ u16_buf_pos +1];
         fs_g_nav_entry.u32_pos_in_file+=2;
      }
      else
      {
         u16_byte = fs_g_sector[ u16_buf_pos ];
      fs_g_nav_entry.u32_pos_in_file++;
      }
      return u16_byte;
   }
   return FS_EOF;   // No read data
}


#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
//! This fonction return a continue physical memory segment corresponding at a file to write
//!
//! @param     segment  Pointer on the segment structure: <br>
//!                     ->u32_size_or_pos    IN,   shall containt the maximum number of sector to write in file (0 = maximum) <br>
//!                     ->u32_size_or_pos    OUT,  containt the segment size (unit sector) <br>
//!                     ->other              IN,   ignored <br>
//!                     ->other              OUT,  containt the segment position <br>
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! This routine is interesting to write a file via a DMA and avoid the file system decode
//! because this routine return a physical memory segment without File System data.
//! Note: the file can be fragmented and it can be necessary to call several time file_write().
//! @endverbatim
//!
Bool  file_write( Fs_file_segment _MEM_TYPE_SLOW_ *segment )
{
   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_WRITE_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_READ_ONLY;
      return FALSE;
   }

   if( !fat_write_file( FS_CLUST_ACT_SEG , segment->u32_size_or_pos ))
      return FALSE;

   // Truncate the segment found if more larger than size asked
   if( (segment->u32_size_or_pos != 0)  // if not undefine limit
   &&  (segment->u32_size_or_pos < fs_g_seg.u32_size_or_pos) )
   {
      fs_g_seg.u32_size_or_pos = segment->u32_size_or_pos ;
   }
   
   // Update position file
   fs_g_nav_entry.u32_pos_in_file += ((U32)fs_g_seg.u32_size_or_pos * FS_SIZE_OF_SECTOR);
   
   // Update the file size
   if( fs_g_nav_entry.u32_pos_in_file > fs_g_nav_entry.u32_size )
   {
      fs_g_nav_entry.u32_size = fs_g_nav_entry.u32_pos_in_file;
   }
   file_load_segment_value( segment );
   return TRUE;
}


//! This fonction set the end of file at the current position
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! This routine is use after the last file_write() call.
//! The file_write() routine work on sector (512B) unit,
//! and you can set a Byte size with file_seek() and this routine.
//! @endverbatim
//!
Bool  file_set_eof( void )
{
   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_WRITE_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_READ_ONLY;
      return FALSE;
   }

   // Update the file size
   fs_g_nav_entry.u32_size = fs_g_nav_entry.u32_pos_in_file;

   if( !fat_read_file( FS_CLUST_ACT_CLR ))
      return FALSE;

   return fat_cache_flush();
}


//! This fonction copy the data buffer in file at the current position
//!
//! @param     buffer         data buffer
//! @param     u16_buf_size   data size
//!
//! @return    number of byte write
//! @return    0, in case of error
//!
U16   file_write_buf( U8 _MEM_TYPE_SLOW_ *buffer , U16 u16_buf_size )
{
   _MEM_TYPE_FAST_ U16 u16_nb_write_tmp;
   _MEM_TYPE_FAST_ U16 u16_nb_write;
   _MEM_TYPE_FAST_ U16 u16_pos_in_sector;

   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_WRITE_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_READ_ONLY;
      return FALSE;
   }

   u16_nb_write = 0;  // Init to "no data read"

   while( 0 != u16_buf_size )
   {
      u16_pos_in_sector = fs_g_nav_entry.u32_pos_in_file % FS_SIZE_OF_SECTOR;
      if( (0== u16_pos_in_sector)
      &&  (FS_SIZE_OF_SECTOR <= u16_buf_size))
      {
         u16_nb_write_tmp = u16_buf_size / FS_SIZE_OF_SECTOR;  // read a modulo sector size

         // It is possible to read more than sector
         if( !fat_write_file( FS_CLUST_ACT_SEG , u16_nb_write_tmp ))
            return FALSE;
         
         // Truncate the segment found if more larger than size asked
         if( u16_nb_write_tmp < fs_g_seg.u32_size_or_pos)
         {
            fs_g_seg.u32_size_or_pos = u16_nb_write_tmp;
         }else{
            u16_nb_write_tmp = fs_g_seg.u32_size_or_pos;
         }
   
         // Fill buffer directly
         while( 0 != fs_g_seg.u32_size_or_pos )
         {
            if( CTRL_GOOD != ram_2_memory( fs_g_nav.u8_lun  , fs_g_seg.u32_addr, buffer))
            {
               fs_g_status = FS_ERR_HW;
               return u16_nb_write;
            }
            fs_g_seg.u32_size_or_pos--;
            fs_g_seg.u32_addr++;
            buffer += FS_SIZE_OF_SECTOR;
         }
         // Translate to Byte unit
         u16_nb_write_tmp *= FS_SIZE_OF_SECTOR;
      }
      else
      {
         if( !fat_write_file( FS_CLUST_ACT_ONE  , 1 ))
            return FALSE;

         // Write the data into the cache
         fat_cache_sector_is_modify();
                
         // Compute the number of data to transfer
         u16_nb_write_tmp = FS_SIZE_OF_SECTOR - u16_pos_in_sector;  // Compute the maximum to next sector limit
         if( u16_nb_write_tmp > u16_buf_size )
            u16_nb_write_tmp = u16_buf_size;

         // Fill buffer
         memcpy_ram2ram( &fs_g_sector[ u16_pos_in_sector ], buffer , u16_nb_write_tmp );
         buffer += u16_nb_write_tmp;
      }
      // Update positions
      fs_g_nav_entry.u32_pos_in_file+= u16_nb_write_tmp;
      u16_nb_write                  += u16_nb_write_tmp;
      u16_buf_size                  -= u16_nb_write_tmp;

      // Update the file size
      if( fs_g_nav_entry.u32_pos_in_file > fs_g_nav_entry.u32_size )
      {
         fs_g_nav_entry.u32_size = fs_g_nav_entry.u32_pos_in_file;
      }
   }
   return u16_nb_write;  // All buffer is writed
}


//! This fonction copy the string in the file at the current position
//!
//! @param     string         string to write (NULL terminate)
//!
//! @return    number of byte write
//! @return    0, in case of error
//!
//! @verbatim
//! Before, if you want write a UNICODE string then call file_string_unicode().
//! @endverbatim
//!
U16   file_puts( U8 _MEM_TYPE_SLOW_ *string )
{
   U16 u16_size_string=0;
   if( g_b_file_unicode )
   {
      while( 0 != ((FS_STR_UNICODE)string )[u16_size_string] ) u16_size_string++;
      return file_write_buf( string, u16_size_string*2 );
   }else{
      while( 0 != string[u16_size_string] ) u16_size_string++;
      return file_write_buf( string, u16_size_string );
   }
}


//! This fonction write a character in the file at current position
//!
//! @param     u16_byte    character to write in the file (ASCII in LSB or UNICODE)
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Before, if you want write a UNICODE character then call file_string_unicode().
//! @endverbatim
//!
Bool  file_putc( U16 u16_byte )
{
   U16 u16_buf_pos;

   if( !fat_check_mount_select_open())
      return FALSE;

   if(!(FOPEN_WRITE_ACCESS & fs_g_nav_entry.u8_open_mode))
   {
      fs_g_status = FS_ERR_READ_ONLY;
      return FALSE;
   }

   if( !fat_write_file( FS_CLUST_ACT_ONE  , 1 ))
      return FALSE;

   // Write the data into the cache
   fat_cache_sector_is_modify();

   u16_buf_pos = fs_g_nav_entry.u32_pos_in_file & FS_MASK_SIZE_OF_SECTOR;

   if( g_b_file_unicode )
   {
      if( 0x01 & u16_buf_pos )
      {
         fs_g_status = FS_ERR_POS_UNICODE_BAD;   // Error in position file
         return FALSE;
      }
      fs_g_sector[ u16_buf_pos ]    = MSB(u16_byte);
      fs_g_sector[ u16_buf_pos +1]  = LSB(u16_byte);
      fs_g_nav_entry.u32_pos_in_file+=2;
   }
   else
   {
      fs_g_sector[ u16_buf_pos ]    = LSB(u16_byte);
   fs_g_nav_entry.u32_pos_in_file++;
   }
   
   // Update the file size
   if( fs_g_nav_entry.u32_pos_in_file > fs_g_nav_entry.u32_size )
   {
      fs_g_nav_entry.u32_size = fs_g_nav_entry.u32_pos_in_file;
   }
   return TRUE;
}
#endif  // FS_LEVEL_FEATURES


//! This fonction return the position in the file
//!
//! @return    Position in file
//!
U32   file_getpos( void )
{
   if( !fat_check_mount_select_open() )
      return 0;

   return fs_g_nav_entry.u32_pos_in_file;
}


//! This fonction change the position in file selected
//!
//! @param     u32_pos     number of byte to seek
//! @param     u8_whence   direction of seek <br>
//!                        FS_SEEK_SET   , start at the beginning and foward <br>
//!                        FS_SEEK_END   , start at the end of file and rewind <br>
//!                        FS_SEEK_CUR_RE, start at the current position and rewind <br>
//!                        FS_SEEK_CUR_FW, start at the current position and foward <br>
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  file_seek( U32 u32_pos , U8 u8_whence )
{
   if( !fat_check_mount_select_open())
      return FALSE;

   switch(u8_whence)
   {
      case FS_SEEK_CUR_RE:
      if( fs_g_nav_entry.u32_pos_in_file < u32_pos )
      {  // Out of the limit
         fs_g_status = FS_ERR_BAD_POS;
         return FALSE;
      }
      // update the position
      fs_g_nav_entry.u32_pos_in_file -= u32_pos;
      break;

      case FS_SEEK_SET:
      if( fs_g_nav_entry.u32_size < u32_pos )
      {  // Out of the limit
         fs_g_status = FS_ERR_BAD_POS;
         return FALSE;
      }
      // update the position
      fs_g_nav_entry.u32_pos_in_file = u32_pos;
      break;

      case FS_SEEK_END:
      if( fs_g_nav_entry.u32_size < u32_pos )
      {  // Out of the limit
         fs_g_status = FS_ERR_BAD_POS;
         return FALSE;
      }
      // update the position
      fs_g_nav_entry.u32_pos_in_file = fs_g_nav_entry.u32_size - u32_pos;
      break;

      case FS_SEEK_CUR_FW:
      u32_pos += fs_g_nav_entry.u32_pos_in_file;
      if( fs_g_nav_entry.u32_size < u32_pos )
      {  // Out of the limit
         fs_g_status = FS_ERR_BAD_POS;
         return FALSE;
      }
      // update the position
      fs_g_nav_entry.u32_pos_in_file = u32_pos;
      break;
   }
   return TRUE;
}


//! This fonction test if the position is at the beginning of file 
//!
//! @return    1     the position is at the beginning of file
//! @return    0     the position isn't at the beginning of file
//! @return    FFh   error
//!
U8    file_bof( void )
{
   if( !fat_check_mount_select_open() )
      return 0xFF;

   return (0 == fs_g_nav_entry.u32_pos_in_file );
}


//! This fonction test if the position is at the end of file
//!
//! @return    1     the position is at the end of file
//! @return    0     the position isn't at the end of file
//! @return    FFh   error
//!
U8    file_eof( void )
{
   if( !fat_check_mount_select_open() )
      return 0xFF;

   return (fs_g_nav_entry.u32_size <= fs_g_nav_entry.u32_pos_in_file );
}


//! This fonction close the file
//!
void  file_close( void )
{
   // If a file is open, then close this one
   if( fat_check_mount_select_open() )
   {
      
#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
      if( FOPEN_WRITE_ACCESS & fs_g_nav_entry.u8_open_mode )
      {
         // Write file informations
         if( !fat_read_dir() )
            return; // error
         fat_write_entry_file();
         fat_cache_flush();  // In case of error during writing data, flush the data before exit fonction
      }
#endif  // FS_LEVEL_FEATURES
      Fat_file_close;
   }
}

