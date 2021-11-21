/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FAT services.
 *
 * This file is a set of rarely-used FAT functions.
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 *                       The example is written for UC3 and EVK1100.
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
#include "fs_com.h"
#include "fat.h"
#include LIB_MEM
#include LIB_CTRLACCESS


//_____ D E F I N I T I O N S ______________________________________________




//_____ D E C L A R A T I O N S ____________________________________________

//! \name Sub routines used by format routine
//! @{
Bool  fat_select_filesystem               ( U8 u8_fat_type , Bool b_MBR );
Bool  fat_write_MBR                       ( void );
Bool  fat_write_PBR                       ( Bool b_MBR );
Bool  fat_clean_zone                      ( Bool b_MBR );
Bool  fat_initialize_fat                  ( void );
//! @}

//! \name Sub routines used by date read-write routines
//! @{
void  fat_translatedate_number_to_ascii   ( FS_STRING sz_date , PTR_CACHE ptr_date , Bool enable_ms );
void  fat_translate_number_to_ascii       ( FS_STRING sz_ascii_number, U8 u8_size_number_ascii, U8 u8_nb_increment );
void  fat_translatedate_ascii_to_number   ( const FS_STRING sz_date , PTR_CACHE ptr_date , Bool enable_ms );
U16   fat_translate_ascii_to_number       ( const FS_STRING sz_ascii_number, U8 u8_size_number_ascii  );
//! @}

//! \name Sub routine used to create a entry file
//! @{
void  fat_create_long_name_entry          ( FS_STRING sz_name , U8 u8_crc , U8 u8_id  );
U8    fat_create_short_entry_name         ( FS_STRING sz_name , FS_STRING short_name , U8 nb , Bool mode  );
U8    fat_find_short_entry_name           ( FS_STRING sz_name  );
Bool  fat_entry_shortname_compare         ( FS_STRING short_name );
U8    fat_check_name                      ( FS_STRING sz_name  );
U8    fat_translate_char_shortname        ( U8 character );
Bool  fat_alloc_entry_free                ( U8 u8_nb_entry );
//! @}




//! This fonction mount a partition file system (FAT12, FAT16 or FAT32) of drive selected
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Global variables used
//! IN : 
//!   fs_g_nav.u8_lun            Indicate the drive to mount    
//!   fs_g_nav.u8_partition      Indicate the partition to mount (if FS_MULTI_PARTITION =  ENABLED )
//! OUT:
//!   fs_g_nav                   update structure
//! @endverbatim
//!
Bool  fat_mount( void )
{
   U8  u8_tmp;
   U16 u16_tmp;
   U32 u32_tmp;

   // the current dir is the root directory
   fs_g_nav.u32_cluster_sel_dir   = 0;
   // No file is selected
   fat_clear_entry_info_and_ptr();

   fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_UNM;
   fs_gu32_addrsector = 0;   // Init ptr fat at the beginning of memory

   // Check if the drive is avialable
   if( !fat_check_device() )
      return FALSE;
      
   while( 1 )  // Check a correct present of the partition selected
   {
      // Read one sector
      if( !fat_cache_read_sector( TRUE ))
         return FALSE;
   
      // Check PBR/MBR signature
      if ( (fs_g_sector[510] != FS_BR_SIGNATURE_LOW  )
      &&   (fs_g_sector[511] != FS_BR_SIGNATURE_HIGH ) )
      {
         fs_g_status = FS_ERR_NO_FORMAT;
         return FALSE;
      }

      if ( 0 == fs_gu32_addrsector )
      {
         //** first sector then check a MBR structure
         // Search the first partition supported
#if (FS_MULTI_PARTITION == ENABLED)
         u16_tmp=0;  // Init to "no valid partition found"
#endif
         for( u8_tmp=0 ; u8_tmp!=4 ; u8_tmp++ )
         {
            // The first sector must be a MBR, then check the partition entry in the MBR
            if ( ((fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+0] == FS_PARTITION_ACTIVE )||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+0] == 0x00                )  )
            &&   ((fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT12           )||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT16_INF32M    )||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT16_SUP32M    )||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT16_SUP32M_BIS)||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT32           )||
                  (fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+4] == FS_PART_TYPE_FAT32_BIS       )) )
            {
               // A valid partition is found
#if (FS_MULTI_PARTITION == ENABLED)
               if( u16_tmp == fs_g_nav.u8_partition )
                  break;   // a valid partition correspond to number partition
               u16_tmp++;
#else
               break;
#endif
            }
         }
         if( u8_tmp != 4 )
         {  
            // Partition found -> Read partition position (in sectors) at offset 8
            LSB0(fs_gu32_addrsector) = fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+8];
            LSB1(fs_gu32_addrsector) = fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+9];
            LSB2(fs_gu32_addrsector) = fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+10];
            LSB3(fs_gu32_addrsector) = fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(u8_tmp)+11];
            continue;   // Go to check PBR of partition
         }   

         // No MBR found then check PBR
#if (FS_MULTI_PARTITION == ENABLED)
         // The device don't have mutli partition, but only one
         if ( 0 != fs_g_nav.u8_partition )
         {
            fs_g_status = FS_ERR_NO_PART;
            return FALSE;
         }
#endif
      }
      
      //** Check a PBR structure
      if ( (fs_g_sector[0] == 0xEB) &&          // PBR Byte 0
           (fs_g_sector[2] == 0x90) &&          // PBR Byte 2
           ((fs_g_sector[21] & 0xF0) == 0xF0) ) // PBR Byte 21 : Media byte
      {  
         break;   // valid PBR found
      }
      // PBR not found
      fs_g_status = FS_ERR_NO_PART;
      return FALSE;
   }

   fs_g_status = FS_ERR_NO_SUPPORT_PART; // by default no support part error

   // Check the BPB_u16BytsPerSec (in bytes)
   if (  ((FS_SIZE_OF_SECTOR & 0xFF) != LOW_16_BPB_BytsPerSec  )
   ||    ((FS_SIZE_OF_SECTOR >> 8)   != HIGH_16_BPB_BytsPerSec )
   ||    ( FS_NB_FAT != U8_BPB_NumFATs ) )   // Check the BPB_NumFATs   
   {
      return FALSE;
   }

   // Read BPB_SecPerClus (unit sector)
   fs_g_nav.u8_BPB_SecPerClus = U8_BPB_SecPerClus;

   //** FAT Type determination (algorithm of "Hardware White Paper FAT")
   // Find the size of FAT (unit sector)
   LSB( u16_tmp ) = LOW_16_BPB_FATSz16;
   MSB( u16_tmp ) = HIGH_16_BPB_FATSz16; 
   if ( 0==u16_tmp )
   {
      LSB( u16_tmp ) = LOW0_32_BPB_FATSz32;
      MSB( u16_tmp ) = LOW1_32_BPB_FATSz32; 
      // a big FAT32 isn't supported by this file system
      if( (0 != LOW2_32_BPB_FATSz32 )
      ||  (0 != LOW3_32_BPB_FATSz32 ) )
      {
         return FALSE;
      }
   }
   fs_g_nav.u16_fat_size = u16_tmp;
   
   // Find the total count of sectors volume
   if ( (0==LOW_16_BPB_TotSec16) && (0==HIGH_16_BPB_TotSec16) )
   {
      LSB0( u32_tmp ) = LOW0_32_BPB_TotSec32;
      LSB1( u32_tmp ) = LOW1_32_BPB_TotSec32; 
      LSB2( u32_tmp ) = LOW2_32_BPB_TotSec32;
      LSB3( u32_tmp ) = LOW3_32_BPB_TotSec32;
   }
   else
   {
      LSB0( u32_tmp ) = LOW_16_BPB_TotSec16;
      LSB1( u32_tmp ) = HIGH_16_BPB_TotSec16; 
      LSB2( u32_tmp ) = 0;
      LSB3( u32_tmp ) = 0;
   }

   // Computed the offset (in sector) between the end of FAT (beginning of root dir in FAT1x) and the beginning of PBR
   fs_g_nav.rootdir.seg.u16_pos = FS_NB_FAT * fs_g_nav.u16_fat_size;

   // Computed the size of root directory (unit sector), for FAT32 is always 0
   LSB( u16_tmp ) = LOW_16_BPB_RootEntCnt;
   MSB( u16_tmp ) = HIGH_16_BPB_RootEntCnt;
   fs_g_nav.rootdir.seg.u16_size = ((u16_tmp * FS_SIZE_FILE_ENTRY) + (FS_SIZE_OF_SECTOR-1)) / FS_SIZE_OF_SECTOR;
   
   // Find the number of sector reserved
   LSB( u16_tmp ) = LOW_16_BPB_ResvSecCnt;
   MSB( u16_tmp ) = HIGH_16_BPB_ResvSecCnt;

   // Computed the address (unit sector) of FAT
   fs_g_nav.u32_ptr_fat = fs_gu32_addrsector + u16_tmp;

   // Computed the offset (in sector) between the first data cluster and the beginning of FAT
   fs_g_nav.u16_offset_data = fs_g_nav.rootdir.seg.u16_pos + fs_g_nav.rootdir.seg.u16_size;

   // Determine the count of SECTORS in the data region of the volume (Total - Sector used)
   u32_tmp -= (u16_tmp + fs_g_nav.u16_offset_data);

   // Determine the count of CLUSTER in the data region of the volume
   // !!!Optimization of -> u32_CountofCluster / U8_BPB_SecPerClus (is a power of 2)
   for( u8_tmp = U8_BPB_SecPerClus ; u8_tmp!=1 ; u8_tmp >>= 1 )
   {
     u32_tmp  >>= 1;   // This computation round down
   }
   fs_g_nav.u32_CountofCluster = u32_tmp+2; // The total of cluster include the two reserved cluster
   
   // Determine the FAT type
   if (u32_tmp < FS_FAT12_MAX_CLUSTERS)
   {
      // Is FAT 12
#if (FS_FAT_12 == DISABLED)
      return FALSE;
#endif
      fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_12;
   } else {
   if (u32_tmp < FS_FAT16_MAX_CLUSTERS)
   {
      // Is FAT 16
#if (FS_FAT_16 == DISABLED)
      return FS_NO_SUPPORT_PART;
#endif
      fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_16;
   } else {
      // Is FAT 32
#if (FS_FAT_32 == DISABLED)
      return FALSE;
#endif
      fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_32;
      // In FAT32, the root dir is file with a start cluster
      LSB0( fs_g_nav.rootdir.u32_cluster ) = LOW0_32_BPB_RootClus;
      LSB1( fs_g_nav.rootdir.u32_cluster ) = LOW1_32_BPB_RootClus; 
      LSB2( fs_g_nav.rootdir.u32_cluster ) = LOW2_32_BPB_RootClus;
      LSB3( fs_g_nav.rootdir.u32_cluster ) = LOW3_32_BPB_RootClus;
   }
   }

   return TRUE;
}



#if (FSFEATURE_WRITE_COMPLET == (FS_LEVEL_FEATURES & FSFEATURE_WRITE_COMPLET) )

//! Global variable to optimize the code of format routines
_MEM_TYPE_SLOW_   U32 fs_s_u32_size_partition;

//! This fonction format the current drive
//!
//! @param     u8_fat_type    Select the type of format <br>
//!            FS_FORMAT_DEFAULT, The file system module choose the better FAT format for the drive space <br>
//!            FS_FORMAT_FAT, The FAT12 or FAT16 is used to format the drive, if possible (disk space <2GB) <br>
//!            FS_FORMAT_FAT32, The FAT32 is used to format the drive, if possible (disk space >32MB) <br>
//!            FS_FORMAT_NOMBR_FLAG, if you don't want a MRB in disk then add this flag (e.g. FAT format on a CD support)
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Global variables used
//! IN : 
//! fs_g_nav.u8_lun        indicate the drive to foramt
//!
//! This routine can't format a multi-partiton, if the disk containt as a multi-partition
//! then the multi-partition is erase and replace by a single partition on ALL disk space.
//! @endverbatim
//!
Bool  fat_format( U8 u8_fat_type )
{
   Bool b_MBR;
#if (FS_MULTI_PARTITION == ENABLED)
#error NOT SUPPORTED
   fs_g_nav.u8_partition = 0;
#endif
   
    
   // Read capacity of drive (last LBA)
   mem_read_capacity( fs_g_nav.u8_lun , &fs_s_u32_size_partition );
 
   if( u8_fat_type & FS_FORMAT_NOMBR_FLAG )
   {
      b_MBR = FALSE;
      u8_fat_type &= ~FS_FORMAT_NOMBR_FLAG;
      // Here size of partition = size disk = last LBA + 1
      fs_s_u32_size_partition++;
   }else{
      b_MBR = TRUE;
      // Here size of partition = size disk -1 = last LBA
   }

   // Initialise the file system for this device
   if( !fat_select_filesystem( u8_fat_type , b_MBR ))
      return FALSE;
   
   // Write the MBR sector (first sector)
   if( b_MBR )
      if( !fat_write_MBR())
         return FALSE;

   // Write the PBR sector
   if( !fat_write_PBR( b_MBR ))
      return FALSE;

   // Clear reserved zone, FAT zone, and Root dir zone
   // Remark: the reserve zone of FAT32 isn't initialized, because BPB_FSInfo is egal to 0
   if( !fat_clean_zone( b_MBR ))
      return FALSE;

   // Initialization of the FAT 1 and 2
   if( !fat_initialize_fat())
      return FALSE;

   // Force the flush of last write sector
   return fat_cache_flush();
}



//! Struture for the tables format
typedef struct st_fs_format_table {
   U32   u32_disk_size;
   U8    u8_SecPerClusVal;
} Fs_format_table;


//! Table format for FAT12
_CONST_TYPE_ Fs_format_table TableFAT12[] = {
   {  4096, 1},      // disks up to 2 MB, 512 bytes cluster
   {  8192, 2},      // disks up to 4 MB, 1k cluster
   { 16384, 4},      // disks up to 8 MB, 2k cluster
   { 32680, 8},      // disks up to 16 MB, 4k cluster
};
/*
NOTE: that this table includes
entries for disk sizes larger than 16 MB even though typically
only the entries for disks < 16 MB in size are used.
The way this table is accessed is to look for the first entry
in the table for which the disk size is less than or equal
to the DiskSize field in that table entry. For this table to
work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
must be 2, and BPB_RootEntCnt must be 512.
*/


//! Table format for FAT16
_CONST_TYPE_ Fs_format_table TableFAT16[] = {
   { 8400, 0},       // disks up to 4.1 MB, the 0 value for SecPerClusVal trips an error
   { 32680, 2},      // disks up to 16 MB, 1k cluster
   { 262144, 4},     // disks up to 128 MB, 2k cluster
   { 524288, 8},     // disks up to 256 MB, 4k cluster
   { 1048576, 16},   // disks up to 512 MB, 8k cluster
   // The entries after this point are not used unless FAT16 is forced
   { 2097152, 32},   // disks up to 1 GB, 16k cluster
   { 4194304, 64},   // disks up to 2 GB, 32k cluster
   { 0xFFFFFFFF, 0}  // any disk greater than 2GB, 0 value for SecPerClusVal trips an error
};
/*
NOTE: that this table includes
entries for disk sizes larger than 512 MB even though typically
only the entries for disks < 512 MB in size are used.
The way this table is accessed is to look for the first entry
in the table for which the disk size is less than or equal
to the DiskSize field in that table entry. For this table to
work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
must be 2, and BPB_RootEntCnt must be 512. Any of these values
being different may require the first table entries DiskSize value
to be changed otherwise the cluster count may be to low for FAT16.
*/


//! Table format for FAT32
_CONST_TYPE_ Fs_format_table TableFAT32[] = {
   { 66600, 0},      // disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error
   { 532480, 1},     // disks up to 260 MB, .5k cluster
   { 16777216, 8},   // disks up to 8 GB, 4k cluster
   { 33554432, 16},  // disks up to 16 GB, 8k cluster
   { 67108864, 32},  // disks up to 32 GB, 16k cluster
   { 0xFFFFFFFF, 64} // disks greater than 32GB, 32k cluster
};
/*
NOTE: that this table includes
entries for disk sizes smaller than 512 MB even though typically
only the entries for disks >= 512 MB in size are used.
The way this table is accessed is to look for the first entry
in the table for which the disk size is less than or equal
to the DiskSize field in that table entry. For this table to
work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
must be 2. Any of these values being different may require the first
table entries DiskSize value to be changed otherwise the cluster count
may be to low for FAT32.
*/



//! This fonction compute the file system type to use for the disk selected
//!
//! @param     u8_fat_type    Select the type of format <br>
//!            FS_FORMAT_DEFAULT, The file system module choose the better FAT format for the drive space <br>
//!            FS_FORMAT_FAT, The FAT12 or FAT16 is used to format the drive, if possible (disk space <2GB) <br>
//!            FS_FORMAT_FAT32, The FAT32 is used to format the drive, if possible (disk space >32MB) <br>
//! @param     b_MBR          TRUE, include a MBR on disk
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Compute the fat type, fat position and fat size.
//! @endverbatim
//!
Bool  fat_select_filesystem( U8 u8_fat_type , Bool b_MBR )
{
   U8 u8_i;
   U8 u8_tmp = 0;
   U16  u16_tmp, u16_tmp2;
   Fs_format_table _CONST_TYPE_ *ptr_table;
  
   //** Check if the default system are selected
   if( (FS_FORMAT_FAT   != u8_fat_type )
   &&  (FS_FORMAT_FAT32 != u8_fat_type ) )
   {  // It is the default format option
      if( (((U32)512*1024*1024)/FS_SIZE_OF_SECTOR) >= fs_s_u32_size_partition  )
      {
         u8_fat_type = FS_FORMAT_FAT;
      } else {
         u8_fat_type = FS_FORMAT_FAT32;
      }
   }
   
   //** Check the file system choice
   if(FS_FORMAT_FAT == u8_fat_type )
   {  
      if( (((U32)15*1024*1024)/FS_SIZE_OF_SECTOR) >= fs_s_u32_size_partition  )
      {
         // FAT 12 format
         fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_12;
         u8_i = sizeof(TableFAT12);
         ptr_table = TableFAT12;
      }else{
         // FAT 16 format
         fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_16;
         u8_i = sizeof(TableFAT16);
         ptr_table = TableFAT16;
      }
   }
   else
   {  // FAT 32 format
      fs_g_nav_fast.u8_type_fat = FS_TYPE_FAT_32;
      u8_i = sizeof(TableFAT32);
      ptr_table = TableFAT32;
   }
   for(  ; u8_i!=0 ; u8_i-- )
   {
      if( fs_s_u32_size_partition <= ptr_table->u32_disk_size )
      {
         // Size of cluster (unit sector)
         fs_g_nav.u8_BPB_SecPerClus = ptr_table->u8_SecPerClusVal;
         break;
      }
      ptr_table++;
   }
   if(0 == fs_g_nav.u8_BPB_SecPerClus)
   {
      fs_g_status = FS_ERR_BAD_SIZE_FAT;    // The disk size is smaller than 4,1MB, or not supported by file system selected
      return FALSE;
   }
   
   //** Compute fat size
   // Compute PBR address
   if( b_MBR )
      fs_g_nav.u32_ptr_fat = 1;  // MBR exist
   else
      fs_g_nav.u32_ptr_fat = 0;  // no MBR

   if( Is_fat12 )
   {  // FAT 12     
      fs_g_nav.u32_ptr_fat += 1;  // FAT address = PBR address + 1
      // Try all possibility of FAT12 size
      fs_g_nav.u16_fat_size=1;
      while(1)
      {
         if( 12 < fs_g_nav.u16_fat_size)   // Max FAT size in FAT12 mode (unit sector) (=0xFFE*1.5/FS_SIZE_OF_SECTOR)
         {
            fs_g_status = FS_ERR_BAD_SIZE_FAT;    // The disk size is smaller than 4,1MB, or not supported by file system selected
            return FALSE;
         }
         // Compare the number of cluster of data zone
         // Note: -1 to not compute PBR sector
         u16_tmp  = ((fs_s_u32_size_partition -1 - (fs_g_nav.u16_fat_size *2)) / fs_g_nav.u8_BPB_SecPerClus)+2;
         u16_tmp2 = (fs_g_nav.u16_fat_size *FS_SIZE_OF_SECTOR *2) / 3;
         if( u16_tmp <= u16_tmp2 )
            break;   // FAT size OK

         fs_g_nav.u16_fat_size++;
      }
   }
   else
   {
      if( Is_fat32 )
      {  // FAT 32
         fs_g_nav.u32_ptr_fat += 32;  // FAT address = PBR address + BPB_ResvSecCnt
         // RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec – 1)) / BPB_BytsPerSec;
         //                = (FS_SIZE_OF_SECTOR-1) / FS_SIZE_OF_SECTOR = 0
         // TmpVal1        = DskSize – (BPB_ResvdSecCnt + RootDirSectors);
         //                = fs_s_u32_size_partition - (32 + 0)
         //                = fs_s_u32_size_partition - u8_tmp
         u8_tmp = 32;
         // TmpVal2        = ((256 * BPB_SecPerClus) + BPB_NumFATs )/2;
         //                = ((((U16)fs_g_nav.u8_BPB_SecPerClus)<<8) + 2) >> 1;
         //                = (((U16)fs_g_nav.u8_BPB_SecPerClus)<<7) + 1;
         //                = u16_tmp
         u16_tmp = (((U16)fs_g_nav.u8_BPB_SecPerClus)<<7) + 1;
         // BPB_FATSz16    = 0;
         // BPB_FATSz32    = FATSz;
      }
      if( Is_fat16 )
      {  // FAT 16
         fs_g_nav.u32_ptr_fat += 1;  // FAT address = PBR address + BPB_ResvSecCnt
         // RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec – 1))  / BPB_BytsPerSec
         //                = ((512            * 32) + (FS_SIZE_OF_SECTOR-1)) / FS_SIZE_OF_SECTOR
         //                = 32
         // TmpVal1        = DskSize – (BPB_ResvdSecCnt + RootDirSectors);
         //                = fs_s_u32_size_partition - (1 + 32)
         //                = fs_s_u32_size_partition - u8_tmp
         u8_tmp = 33;
         // TmpVal2        = ((256 * BPB_SecPerClus) + BPB_NumFATs )/2;
         //                = (((U16)fs_g_nav.u8_BPB_SecPerClus)<<8) + 2;
         //                = u16_tmp
         MSB(u16_tmp) = fs_g_nav.u8_BPB_SecPerClus;
         LSB(u16_tmp) = 2;
      }
      // FATSz          = (TMPVal1 + TmpVal2 – 1) / TmpVal2;
      fs_g_nav.u16_fat_size = (fs_s_u32_size_partition -u8_tmp +u16_tmp -1) / u16_tmp;
   }
   
   return TRUE;
}


//! This fonction write the MBR
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_write_MBR( void )
{
   U8 u8_i = 0;
        
   // Init and reset the cache at the beginning of memory
   fs_gu32_addrsector = 0;
   if( !fat_cache_read_sector( FALSE ))
      return FALSE;
   fat_cache_sector_is_modify();
   fat_cache_clear();
   
   // MBR signature
   fs_g_sector[510] = FS_BR_SIGNATURE_LOW;
   fs_g_sector[511] = FS_BR_SIGNATURE_HIGH;

   // Write the partition entry in the MBR
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +0] = FS_PARTITION_ACTIVE;   // Active partition
   // Remark: cylinder and header start to 0, and sector value start to 1
   //fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +1] = 0;                   // The head (0) where the partition starts
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +2] = 2;                     // The sector (2=next to MBR) and the cylinder (0) where the partition starts
   //fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +3] = 0;
   
   // Write patition type
   if( Is_fat32 )
   {  // FAT 32
      u8_i = FS_PART_TYPE_FAT32;
   }
   if( Is_fat16 )
   {  // FAT 16
      if( fs_s_u32_size_partition < (32L*1024*(1024/FS_SIZE_OF_SECTOR)) )
      {  // Disk inferieur to 32MB
         u8_i = FS_PART_TYPE_FAT16_INF32M;
      }else{
         u8_i = FS_PART_TYPE_FAT16_SUP32M;
      }
   }
   if( Is_fat12 )
   {  // FAT 12
      u8_i = FS_PART_TYPE_FAT12;
   }

   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +4] = u8_i;
   
   // Specific compute
   // The head where the partitions ends
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +5] = (LSB1(fs_s_u32_size_partition)<<2) + (LSB0(fs_s_u32_size_partition)>>6);
   // The sector and the cylinder where the partition ends
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +6] = (LSB1(fs_s_u32_size_partition)&0xC0) + (LSB0(fs_s_u32_size_partition)&0x3F);
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0) +7] = LSB2(fs_s_u32_size_partition);
   
   // Write partition position (in sectors) at offset 8
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+ 8] = 0x01;
   //fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+ 9] = 0x00;
   //fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+10] = 0x00;
   //fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+11] = 0x00;
   // Write the number of sector in partion (= size - one sector MBR = last LBA, return by read_capacity)
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+12] = LSB0(fs_s_u32_size_partition);
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+13] = LSB1(fs_s_u32_size_partition);
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+14] = LSB2(fs_s_u32_size_partition);
   fs_g_sector[FS_MBR_OFFSET_PART_ENTRY(0)+15] = LSB3(fs_s_u32_size_partition);

   return TRUE;
}

//! Constante for fat_write_PBR() routine
_CONST_TYPE_ U8 const_header_pbr[] = {
   0xEB,0,0x90,                                       // offset 00-02, Add jump boot flag
   'M','S','W','I','N','4','.','1',                   // offset 03-11, No add OEM name
   FS_SIZE_OF_SECTOR & 0xFF, FS_SIZE_OF_SECTOR >>8,   // offset 11-12, Add byte per sector
   0,                                                 // offset 13-13, Add sector by cluster
   0,0,                                               // offset 14-15, Add Number of reserved sector (see next step for optimization test)
   2,                                                 // offset 16-16, Add Number of FAT
   0,0,                                               // offset 17-18, Add Number of root entry (FAT32 = 0 entry, FAT16 = 512 entrys)
   0,0,
   FS_PART_NO_REMOVE_MEDIA,                           // offset 21-21, Media byte
   0,0,
   0x3F,0,                                            // offset 24-25, Sector per track (must be egal to MBR information, also maximum sector per head = 0x3F = 6bits)
   0,0,                                               // offset 26-27, Number of header
   1                                                  // offset 28-31, Number of hidden setors
   };
_CONST_TYPE_ U8 const_tail_pbr[] = {           // offset 36 on FAT 16, offset 64 on FAT 32
   FS_PART_HARD_DISK,                                 // Driver number
   0,                                                 // Reserved (used by Windows NT)
   FS_BOOT_SIGN,                                      // Extended boot signature
   0,0,0,0,                                           // volume ID
   'N','O',' ','N','A','M','E',' ',' ',' ',' ',       // volume label (11 characters);
   'F','A','T',' ',' ',' ',' ',' ',                   // File system type in ASCII (8 characters);
   };


//! \name Constante of "FAT32 FSInfo Sector"
//! @{
_CONST_TYPE_ U8 const_FSI_LeadSig[] = {
   0x52,0x52,0x61,0x41                                //! offset 00-04, This lead signature
};
_CONST_TYPE_ U8 const_FSI_StrucSig[] = {
   0x72,0x72,0x41,0x61                                //! offset 484-487, signature
};
//! @}


//! This fonction write the PBR
//!
//! @param     b_MBR          TRUE, include a MBR on disk
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_write_PBR( Bool b_MBR )
{
   U16 u16_tmp;
   
   //** Init the cache sector with PBR
   if( b_MBR )
      fs_gu32_addrsector = 1;
   else
      fs_gu32_addrsector = 0;

   if( !fat_cache_read_sector( FALSE ))
      return FALSE;
   fat_cache_sector_is_modify();
   fat_cache_clear();
  
   //** WRITE CONSTANTE & VARIABLE FOR FAT and FAT32
   memcpy_code2ram( fs_g_sector, const_header_pbr , sizeof(const_header_pbr) );
   // PBR signature
   fs_g_sector[510] = FS_BR_SIGNATURE_LOW;
   fs_g_sector[511] = FS_BR_SIGNATURE_HIGH;

   // offset 13-13, Add sector by cluster
   fs_g_sector[13] = fs_g_nav.u8_BPB_SecPerClus;
   // offset 26-27, Number of header
   fs_g_sector[26] = (LSB1(fs_s_u32_size_partition)<<2) + (LSB0(fs_s_u32_size_partition)>>6);
  
   //** WRITE CONSTANTE & VARIABLE DEPENDING OF FAT16 and FAT32
   // Since offset 36, there are a different structure space for FAT16 and FAT32
   // offset 39-42 or 67-70, Volume ID not used
   // offset 43-53 or 71-81, Volume Label
   // offset 54-61 or 82-89, File system type
   if( Is_fat32 )
   {
      memcpy_code2ram( &fs_g_sector[64], const_tail_pbr, sizeof(const_tail_pbr) );
   }else{
      memcpy_code2ram( &fs_g_sector[36], const_tail_pbr, sizeof(const_tail_pbr) );
   }
   
   u16_tmp = fs_g_nav.u16_fat_size;    // save value in fast data space for optimization code
   if( Is_fat32 )
   {
      // offset 14-15, Add Number of reserved sector, FAT32 = 32 sectors
      fs_g_sector[14] = 32;
      // offset 17-18, Add Number of root entry, FAT32 = 0 entry
      // offset 36-39, Fat size 32bits
      LOW0_32_BPB_FATSz32 = LSB(u16_tmp);
      LOW1_32_BPB_FATSz32 = MSB(u16_tmp); 
      // offset 40-41, Ext flags (all FAT are enabled = 0)
      // offset 42-43, Fs version (version0:0 = 0)
      // offset 44-47, Root Cluster (first free cluster = 2)
      fs_g_sector[44]= 2;
      // offset 48-49, Fs Info (usualy 1)
      // fs_g_sector[48]= 0;
      // offset 50-51, Backup Boot Sector (usualy 6)
      // fs_g_sector[50]= 0;
      // offset 52-63, reserved space
      // offset 54-61, File system type
      fs_g_sector[85]='3';
      fs_g_sector[86]='2';
   }
   else
   {
      // FAT 12 or 16
      // offset 14-15, Add Number of reserved sector, FAT = 1 sector
      fs_g_sector[14] = 1;
      // offset 17-18, Add Number of root entry, FAT = 512 entrys
      //fs_g_sector[17] = 512&0xFF;
      fs_g_sector[18] = 512>>8;

      // offset 22-23, Fat size 16bits
      LOW_16_BPB_FATSz16  = LSB(u16_tmp);
      HIGH_16_BPB_FATSz16 = MSB(u16_tmp); 
      // offset 54-61, File system type
      fs_g_sector[57]='1';
      if( Is_fat12 )
      {
         fs_g_sector[58]='2';
      }else{
         fs_g_sector[58]='6';
      }
   }
    
   // Write the number of sector in partion (= size - one sector MBR = last LBA, return by read_capacity)
   if( ( Is_fat32 )
   ||  ((0x10000-1) <= fs_s_u32_size_partition) )
   {
      // FAT32 or disk > 32MB
      // offset 32-35, Number of sector in partition (value 32 bits)
      fs_g_sector[32] = LSB0(fs_s_u32_size_partition);
      fs_g_sector[33] = LSB1(fs_s_u32_size_partition);
      fs_g_sector[34] = LSB2(fs_s_u32_size_partition);
      fs_g_sector[35] = LSB3(fs_s_u32_size_partition);
   }
   else
   {
      // offset 19-20, Number of sector in partition (value 16 bits)
      fs_g_sector[19] = LSB0(fs_s_u32_size_partition);
      fs_g_sector[20] = LSB1(fs_s_u32_size_partition);
   }
      
   if( Is_fat32 )
   {
      // Init the FAT32 FSInfo Sector
      if( !fat_write_fat32_FSInfo( 0xFFFFFFFF ))
         return FALSE;
   }
   return TRUE;
}


#ifdef  FS_FAT_32
//! This fonction write the space free number in drive partition FAT32 selected
//!
//! Read global value "fs_g_status" in case of error :
//!          FS_ERR_HW                Hardware driver error
//!          FS_ERR_HW_NO_PRESENT     Device not present
//!          FS_LUN_WP            Drive is read only
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_write_fat32_FSInfo( U32 u32_nb_free_cluster )
{
   // Init sector
   fs_gu32_addrsector = fs_g_nav.u32_ptr_fat - 31;
   if( !fat_cache_read_sector( FALSE ))
      return FALSE;
   fat_cache_sector_is_modify();
   fat_cache_clear();
   
   // Fill sector
   // offset 00-04, This lead signature
   memcpy_code2ram( &fs_g_sector[0], const_FSI_LeadSig, sizeof(const_FSI_LeadSig) );
   // offset 004-483, reserved (fill with 0)
   // offset 484-487, signature
   memcpy_code2ram( &fs_g_sector[484], const_FSI_StrucSig, sizeof(const_FSI_StrucSig) );
   // offset 488-491, free cluster count (by default NO value)
   fs_g_sector[488] = LSB0(u32_nb_free_cluster);
   fs_g_sector[489] = LSB1(u32_nb_free_cluster);
   fs_g_sector[490] = LSB2(u32_nb_free_cluster);
   fs_g_sector[491] = LSB3(u32_nb_free_cluster);
   // offset 492-495, indicates the cluster number at which the driver should start looking for free clusters (by default NO value)
   memset( &fs_g_sector[492] , 0xFF , 4 );
   // offset 496-509, reserved (fill with 0)
   // offset 510-511, Signature
   fs_g_sector[510] = FS_BR_SIGNATURE_LOW;
   fs_g_sector[511] = FS_BR_SIGNATURE_HIGH;
   return TRUE;
}


//! This fonction return the space free in the drive partition FAT32 selected
//!
//! @return the number of sector free (if 0xFFFFFFFF, then no value available in FSInfo Sector)
//!
U32   fat_read_fat32_FSInfo( void )
{
   U32 u32_nb_free_cluster;

   //* Read FAT32 FSInfo Sector
   fs_gu32_addrsector = fs_g_nav.u32_ptr_fat - 31;
   if( !fat_cache_read_sector( TRUE ))
      return FALSE;
   
   //* Check signature
   // offset 510-511, Signature
   if( fs_g_sector[510] != FS_BR_SIGNATURE_LOW )
      return 0xFFFFFFFF;
   if( fs_g_sector[511] != FS_BR_SIGNATURE_HIGH)
      return 0xFFFFFFFF;
   // offset 00-04, This lead signature
   if( 0 != memcmp_code2ram( &fs_g_sector[0], const_FSI_LeadSig, sizeof(const_FSI_LeadSig) ))
      return 0xFFFFFFFF;
   // offset 004-483, reserved (fill with 0)
   // offset 484-487, signature
   if( 0 != memcmp_code2ram( &fs_g_sector[484], const_FSI_StrucSig, sizeof(const_FSI_StrucSig)) )
      return 0xFFFFFFFF;
   
   //* Read value
   // offset 488-491, free cluster count
   LSB0(u32_nb_free_cluster) = fs_g_sector[488];
   LSB1(u32_nb_free_cluster) = fs_g_sector[489];
   LSB2(u32_nb_free_cluster) = fs_g_sector[490];
   LSB3(u32_nb_free_cluster) = fs_g_sector[491];
   return u32_nb_free_cluster;
}
#endif  // FS_FAT_32


//! This fonction clean the reserved zone, FAT zone, and root dir zone
//!
//! @param     b_MBR          TRUE, include a MBR on disk
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_clean_zone( Bool b_MBR )
{
   U16 u16_nb_sector_clean;
   
   // Force the flush before clear the buffer
   if( !fat_cache_flush())
      return FALSE;
   fat_cache_clear();
   
   // remark: this zones are storage after the PBR and are continue
   // Start after PBR
   if( b_MBR )
   {
      fs_gu32_addrsector = 2; // Jump MBR and PBR
   }else{
      fs_gu32_addrsector = 1; // Jump only a PBR (no MBR create)
   }
   
   // Compute reserved zone an root dir zone
   if( Is_fat32 )
   {  // FAT 32
      fs_gu32_addrsector++;   // Jump FAT32 FSInfo Sector
      // root dir size = cluster size, reserved zone = 32 - 2 (2 = PBR + FSInfo)
      u16_nb_sector_clean = fs_g_nav.u8_BPB_SecPerClus + 30;
   }
   else
   {  // FAT 12 or 16
      // root dir size = 512 entrys = 32 sectors, reserved zone = 1 - 1(PBR)
      u16_nb_sector_clean = 32;
   }
   u16_nb_sector_clean += (fs_g_nav.u16_fat_size*2);  // Add FAT size

   // loop to clean
   for( ; u16_nb_sector_clean!=0; u16_nb_sector_clean-- )
   {
      if( !fat_cache_read_sector( FALSE ))
         return FALSE;
      fat_cache_sector_is_modify();
      fs_gu32_addrsector++;   // Next sector
   }
   return TRUE;
}


//! \name Constante for fat_initialize_fat() fonction
//! @{
_CONST_TYPE_ U8 const_header_fat12[] = {
   0xF8,0xFF,0xFF                            // reserved clusters 0 & 1
   };
_CONST_TYPE_ U8 const_header_fat16[] = {
   0xF8,0xFF,0xFF,0xFF                       // reserved clusters 0 & 1
   };
_CONST_TYPE_ U8 const_header_fat32[] = {
   0xF8,0xFF,0xFF,0x0F,0xFF,0xFF,0xFF,0x0F   // reserved clusters 0 & 1
  ,0xFF,0xFF,0xFF,0x0F                       // reserved clusters 2 for root directory
   };
//! @}


//! This fonction initialize the fat one and two
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_initialize_fat( void )
{
   // Init and reset the cache at the beginning of memory
   fs_gu32_addrsector = fs_g_nav.u32_ptr_fat;
   if( !fat_cache_read_sector( FALSE ))
      return FALSE;
   fat_cache_sector_is_modify();

   if( Is_fat32 )
   {
      memcpy_code2ram( fs_g_sector, const_header_fat32, sizeof(const_header_fat32) );
   }
   if( Is_fat16 )
   {
      memcpy_code2ram( fs_g_sector, const_header_fat16, sizeof(const_header_fat16) );
   }
   if( Is_fat12 )
   {
      memcpy_code2ram( fs_g_sector, const_header_fat12, sizeof(const_header_fat12) );
   }

   // copy this first sector FAT in second FAT
   fs_gu32_addrsector = fs_g_nav.u32_ptr_fat + fs_g_nav.u16_fat_size;
   if( !fat_cache_read_sector( FALSE ))
      return FALSE;
   fat_cache_sector_is_modify();
   return TRUE;
}


//! This fonction to read or to write a serial number
//!
//! @param     b_action  choose action (FS_SN_READ or FS_SN_WRITE)
//! @param     a_u8_sn   array to store or get the serial number (U8[4])
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_serialnumber( Bool b_action , U8 _MEM_TYPE_SLOW_ *a_u8_sn )
{
   // compute address of PBR sector
   if ( Is_fat12 || Is_fat16 )
   {  // FAT 12 & 16 (BPB_ResvSecCnt must be ega to 1)
      fs_gu32_addrsector = fs_g_nav.u32_ptr_fat-1;
   }
   else
   {  // FAT 32 (BPB_ResvSecCnt must be ega to 32)
      fs_gu32_addrsector = fs_g_nav.u32_ptr_fat-32;
   }
   // Read PBR
   if( !fat_cache_read_sector( TRUE ))
      return FALSE;

   // The serial is storage at invert
   if( b_action == FS_SN_READ )
   {
      // Read serial number
      a_u8_sn[0] = fs_g_sector[42];
      a_u8_sn[1] = fs_g_sector[41];
      a_u8_sn[2] = fs_g_sector[40];
      a_u8_sn[3] = fs_g_sector[39];
      return TRUE;
   }else{
      // Write serial number
      fs_g_sector[42] = a_u8_sn[0];
      fs_g_sector[41] = a_u8_sn[1];
      fs_g_sector[40] = a_u8_sn[2];
      fs_g_sector[39] = a_u8_sn[3];
      fat_cache_sector_is_modify();
      return fat_cache_flush();
   }
}


//! This fonction to read or to write a label
//!
//! @param     b_action    choose action (FS_LABEL_READ or FS_LABEL_WRITE)
//! @param     sz_label    string ASCII to store labe (11B min) or to get new label (11B max)
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_entry_label( Bool b_action , FS_STRING sz_label )
{
   PTR_CACHE ptr_entry;
   U8 u8_pos_name;
   U8 u8_char;
   
   ptr_entry = fat_get_ptr_entry();
   
   if( FS_LABEL_READ == b_action)
   {   
      if( FS_ENTRY_END == ptr_entry[0] )          // end of directory
      {
         fs_g_status = FS_ERR_ENTRY_EMPTY;
         return FALSE;
      }
      if( FS_ATTR_VOLUME_ID != ptr_entry[11])   // no system label
      {
         fs_g_status = FS_ERR_ENTRY_BAD;
         return FALSE;
      }
      if( NULL == sz_label )
         return TRUE;
      for( u8_pos_name=0; u8_pos_name<11; u8_pos_name++ )
      {  
         u8_char = *ptr_entry;
         if( 0x20 == u8_char )
            u8_char = 0;
         *sz_label = u8_char; 
         if( 0 == u8_char )
            return TRUE;   // Finish to read name
         ptr_entry++;
         sz_label++;
      }
      *sz_label = 0; 
      return TRUE;
   }

   if( FS_LABEL_WRITE == b_action)
   {   
      // Autorize write label only on empty entry or system label entry
      if( FS_ENTRY_END != ptr_entry[0] )          // end of directory
      {
         if( FS_ATTR_VOLUME_ID != ptr_entry[11])   // no system label
         {
            fs_g_status = FS_ERR_ENTRY_BAD;
            return FALSE;
         }
      }
      if( 0 == *sz_label )
      {
         fs_g_status = FS_ERR_NAME_INCORRECT;
         return FALSE;
      }
      ptr_entry[11] = FS_ATTR_VOLUME_ID;
      for( u8_pos_name=0; u8_pos_name<11; u8_pos_name++ )
      {  
         u8_char = *sz_label; 
         if( ('a'<=u8_char) && (u8_char<='z') )
         {
            u8_char -= ('a'-'A');   // Change to upper case
         }
         if( 0 == u8_char )
         {
            u8_char = 0x20;
         }
         *ptr_entry = u8_char;
         ptr_entry++;
         if( 0x20 == u8_char )
            continue;
         sz_label++;
      }
      fat_cache_sector_is_modify();
      return fat_cache_flush();
   }
   return FALSE;  // To del a warning compilation
}
#endif  // FS_LEVEL_FEATURES


#if (FSFEATURE_WRITE_COMPLET == (FS_LEVEL_FEATURES & FSFEATURE_WRITE_COMPLET))
//! This fonction write the directory informations
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Global variables used
//! IN : 
//!   fs_g_seg.u32_addr          cluster value of a directory
//! OUT:
//!   update the sector cache with init directory datas
//! @endverbatim
//!
Bool  fat_initialize_dir( void )
{
   U8 u8_i;

   if( !fat_clear_cluster())
      return FALSE;
   fat_cache_sector_is_modify();
   // remark: here, the fat clear cluster have open and clear the first sector of cluster

   // Create the dot "." entry, this one is a directory that points to itself
   fs_g_sector[0]='.';
   for( u8_i=1 ; u8_i<11 ; u8_i++ )
      fs_g_sector[u8_i]=' ';
   fs_g_sector[11]=FS_ATTR_DIRECTORY;
   fs_g_sector[26]= LSB0( fs_g_nav_entry.u32_cluster );
   fs_g_sector[27]= LSB1( fs_g_nav_entry.u32_cluster );
   fs_g_sector[20]= LSB2( fs_g_nav_entry.u32_cluster );
   fs_g_sector[21]= LSB3( fs_g_nav_entry.u32_cluster );
   // Create the dotdot ".." entry, this one points to the starting cluster of the parent directory
   fs_g_sector[FS_SIZE_FILE_ENTRY+0]='.';
   fs_g_sector[FS_SIZE_FILE_ENTRY+1]='.';
   for( u8_i=2 ; u8_i<11 ; u8_i++ )
      fs_g_sector[FS_SIZE_FILE_ENTRY+u8_i]=' ';
   fs_g_sector[FS_SIZE_FILE_ENTRY+11]=FS_ATTR_DIRECTORY;
   fs_g_sector[FS_SIZE_FILE_ENTRY+26]= LSB0( fs_g_nav.u32_cluster_sel_dir );
   fs_g_sector[FS_SIZE_FILE_ENTRY+27]= LSB1( fs_g_nav.u32_cluster_sel_dir );
   fs_g_sector[FS_SIZE_FILE_ENTRY+20]= LSB2( fs_g_nav.u32_cluster_sel_dir );
   fs_g_sector[FS_SIZE_FILE_ENTRY+21]= LSB3( fs_g_nav.u32_cluster_sel_dir );
   
   return TRUE;
}
#endif  // FS_LEVEL_FEATURES


//! This fonction read the informations about the dates
//!
//! @param     type_date         choose date to read (FS_DATE_LAST_WRITE or FS_DATE_CREATION)
//! @param     sz_date           table to store the information about date <br>
//!                              storage format (ASCII) = "YYYYMMDDHHMMSSMS" = year, month, day, hour, minute, seconde, miliseconde
//!
void  fat_get_date( FS_STRING sz_date , Bool type_date )
{
   PTR_CACHE ptr_entry;

   ptr_entry = fat_get_ptr_entry();
   if( FS_DATE_LAST_WRITE == type_date )
   {
      fat_translatedate_number_to_ascii( sz_date , &ptr_entry[22] , FALSE );
   }
   else
   {
      fat_translatedate_number_to_ascii( sz_date , &ptr_entry[13] , TRUE );
   }
}


//! This fonction translate a date and time FAT value to ascii string
//!
//! @param     sz_date           table to store the information about date <br>
//!                              storage format (ASCII) = "YYYYMMDDHHMMSSMS" = year, month, day, hour, minute, seconde, miliseconde
//! @param     ptr_date          point in cache directory sector of the date to translate
//! @param     enable_ms         TRUE, translate the millisecond field
//!
void  fat_translatedate_number_to_ascii( FS_STRING sz_date , PTR_CACHE ptr_date , Bool enable_ms )
{
   FS_STRING ptr_string_date;
   U8 u8_i;
   U8 msb_date, lsb_date, msb_time, lsb_time, u8_ms = 0;

   // Read entry value of date and time
   if( enable_ms )
   {
      u8_ms = *ptr_date;
      ptr_date++;
   }
   lsb_time = *ptr_date;
   ptr_date++;
   msb_time = *ptr_date;
   ptr_date++;
   lsb_date = *ptr_date;
   ptr_date++;
   msb_date = *ptr_date;

   // Initialise the string with "1980000000000000" (Year = 1980 and other at 0)
   ptr_string_date = sz_date;
   *ptr_string_date = '1';
   ptr_string_date++;
   *ptr_string_date = '9';
   ptr_string_date++;
   *ptr_string_date = '8';
   ptr_string_date++;
   for( u8_i=(15-2) ; u8_i!=0 ; u8_i-- )
   {
      *ptr_string_date = '0';
      ptr_string_date++;
   }

   // Get the year
   fat_translate_number_to_ascii( sz_date, 4 , msb_date>>1 );

   // Get the month
   fat_translate_number_to_ascii( &sz_date[4] , 2 , ((msb_date & 0x01)<<3) + (lsb_date>>5) );

   // Get the day
   fat_translate_number_to_ascii( &sz_date[6] , 2 , lsb_date & 0x1F );

   // Get the hour
   fat_translate_number_to_ascii( &sz_date[8] , 2 , msb_time >> (11-8) );

   // Get the minute
   fat_translate_number_to_ascii( &sz_date[10] , 2 , ((msb_time & 0x07)<<3) + (lsb_time>>5) );

   // Get the seconde
   fat_translate_number_to_ascii( &sz_date[12] , 2 , (lsb_time & 0x1F)<<1 );
   if( 99 < u8_ms )
   {
     // Add one seconde
     fat_translate_number_to_ascii( &sz_date[12] , 2 , 1 );
     u8_ms -= 100;
   }

   // Get the miliseconde
   fat_translate_number_to_ascii( &sz_date[14] , 2 , u8_ms );
}


//! This fonction translate a digital number to a ASCII number
//!
//! @param     sz_ascii_number         enter le initiale ascii string to increment (ex:"1907")
//! @param     u8_size_number_ascii    enter le number of digit (ex:4)
//! @param     u8_nb_increment         enter le number of increment (ex:"102")
//!
//! @verbatim
//! OUT, Update sz_ascii_number (ex:"2009")
//! @endverbatim
//!
void  fat_translate_number_to_ascii( FS_STRING sz_ascii_number, U8 u8_size_number_ascii, U8 u8_nb_increment )
{
   FS_STRING ptr_sz_ascii_number;

   u8_size_number_ascii--;

   for( ; u8_nb_increment != 0 ; u8_nb_increment-- )
   {
      ptr_sz_ascii_number = sz_ascii_number + u8_size_number_ascii;
      ptr_sz_ascii_number[0]++;
      while( ('9'+1) == *ptr_sz_ascii_number )
      {
         *ptr_sz_ascii_number = '0';
         ptr_sz_ascii_number--;
         ptr_sz_ascii_number[0]++;
      }
   }
}


#if (FSFEATURE_WRITE_COMPLET == (FS_LEVEL_FEATURES & FSFEATURE_WRITE_COMPLET))
//! This fonction change the informations about date
//!
//! @param     type_date         choose date field (FS_DATE_LAST_WRITE or FS_DATE_CREATION)
//! @param     sz_date           table to store the information about date <br>
//!                              storage format (ASCII) = "YYYYMMDDHHMMSSMS" = year, month, day, hour, minute, seconde, miliseconde
//!
//! @verbatim
//! OUT, update cache sector with the new date
//! @endverbatim
//!
void  fat_set_date( const FS_STRING sz_date , Bool type_date )
{
   PTR_CACHE ptr_entry;

   fat_cache_sector_is_modify();
   ptr_entry = fat_get_ptr_entry();

   if( FS_DATE_LAST_WRITE == type_date )
   {
      fat_translatedate_ascii_to_number( sz_date , &ptr_entry[22] , FALSE );
   }
   else
   {
      fat_translatedate_ascii_to_number( sz_date , &ptr_entry[13] , TRUE );
   }
}


//! This fonction translate a date and time ascii string to FAT value
//!
//! @param     sz_date        table to store the information about date <br>
//!                           storage format (ASCII) = "YYYYMMDDHHMMSSMS" = year, month, day, hour, minute, seconde, miliseconde
//! @param     ptr_date       point in cache directory sector of the date to translate
//! @param     enable_ms      TRUE, translate the millisecond field
//!
//! @verbatim
//! OUT, write the date field at ptr_date
//! @endverbatim
//!
void  fat_translatedate_ascii_to_number( const FS_STRING sz_date , PTR_CACHE ptr_date , Bool enable_ms )
{
   U8 u8_tmp;
   U8 msb_date, lsb_date, msb_time, lsb_time;
   
   // Set the year
   msb_date  = ((U8)(fat_translate_ascii_to_number( sz_date , 4 )-1980))<<1;

   // Set the month
   u8_tmp    = (U8)fat_translate_ascii_to_number( &sz_date[4] , 2 );
   msb_date |= (u8_tmp >> 3); 
   lsb_date  = (u8_tmp << 5); 

   // Set the day
   lsb_date |= (U8)fat_translate_ascii_to_number( &sz_date[6] , 2 );

   // Set the hour
   msb_time  = ((U8)fat_translate_ascii_to_number( &sz_date[8] , 2 )) << (11-8);

   // Set the minute
   u8_tmp    = (U8)fat_translate_ascii_to_number( &sz_date[10] , 2 );
   msb_time |= (u8_tmp >> 3); 
   lsb_time  = (u8_tmp << 5);

   // Set the seconde
   u8_tmp    = (U8)fat_translate_ascii_to_number( &sz_date[12] , 2 );
   lsb_time |= (u8_tmp >> 1);
   
   // Set the miliseconde
   if( enable_ms )
   {
      // check if the seconde time is %2
      if( u8_tmp & 0x01 )
      {  // it isn't %2
         u8_tmp = 100;  // add one seconde
      }
      else
      {
         u8_tmp = 0;    // no more seconde
      }  
      *ptr_date = u8_tmp + (U8)fat_translate_ascii_to_number( &sz_date[14] , 2 );
      ptr_date++;
   }

   // Record value
   ptr_date[0] = lsb_time; 
   ptr_date[1] = msb_time; 
   ptr_date[2] = lsb_date; 
   ptr_date[3] = msb_date; 
}


//! This fonction translate a ASCII number to a digital number
//!
//! @param     sz_ascii_number         enter le initiale ascii string to increment (ex:"1907")
//! @param     u8_size_number_ascii    enter le number of digit (ex:4)
//!
//! @verbatim
//! OUT, update sz_ascii_number
//! @endverbatim
//!
U16   fat_translate_ascii_to_number( const FS_STRING sz_ascii_number, U8 u8_size_number_ascii  )
{
   U8 sz_ascii_number_copy[4];
   U8 _MEM_TYPE_FAST_ *ptr_sz_ascii_number;
   U8 u8_i;
   U16 u16_number;

   for( u8_i=0; u8_i < u8_size_number_ascii; u8_i++ )
   {
      sz_ascii_number_copy[u8_i] = sz_ascii_number[u8_i];
   }

   u16_number=0;

   while( 1 )
   {
      // check if is the end of ascii number (= "0...0")
      ptr_sz_ascii_number = sz_ascii_number_copy;
      for( u8_i = u8_size_number_ascii; u8_i !=0; u8_i-- )
      {
         if( '0' != *ptr_sz_ascii_number )
         {
            break;
         }
         ptr_sz_ascii_number++;
      }
      if( 0 == u8_i)
         return u16_number;
         
      // Decrement the numbers
      ptr_sz_ascii_number = sz_ascii_number_copy + u8_size_number_ascii -1;
      u16_number++;
      ptr_sz_ascii_number[0]--;
      while( ('0'-1) == ptr_sz_ascii_number[0] )
      {
         *ptr_sz_ascii_number = '9';
         ptr_sz_ascii_number--;
         ptr_sz_ascii_number[0]--;
      }
   }
}
#endif  // FS_LEVEL_FEATURES



#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
//! This fonction create a entry file with only the name fields
//!
//! @param     sz_name      name to create (ASCII or UNICODE)
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! IN:
//! b_unicode is a global flag to select UNICODE or ASCII
//! The name must be terminate by NULL and it can't have two '.' character.
//! @endverbatim
//!
Bool  fat_create_entry_file_name( FS_STRING sz_name )
{
   U8 u8_i, u8_nb;
   U8 u8_crc, u8_nb_entry;

   // Compute the number of entry for this name
   u8_nb_entry = fat_check_name( sz_name  );
   if( 0 == u8_nb_entry )
   {
      fs_g_status = FS_ERR_INCORRECT_NAME;
      return FALSE;
   }
   
   // search a unik short entry
   u8_nb = fat_find_short_entry_name( sz_name  );
   
   // Alloc a space for the entrys
   if( !fat_alloc_entry_free( u8_nb_entry ))
      return FALSE;
   // Remark: here the pointer of entry is on the last free entry of new space entry

   // Add short name entry
   u8_crc = fat_create_short_entry_name( sz_name , 0 , u8_nb, FALSE  );
   u8_nb_entry--;
   
   // For each long name entry
   for( u8_i=1 ; u8_i<=u8_nb_entry ; u8_i++ )
   {
      // Go to previous entry
      fs_g_nav_fast.u16_entry_pos_sel_file--;
      if( !fat_read_dir())
         return FALSE;
      // create a long name entry
      if( u8_i == u8_nb_entry )
      {
         u8_i += FS_ENTRY_LFN_LAST;
      }
      fat_create_long_name_entry( sz_name , u8_crc , u8_i );
      sz_name += FS_SIZE_LFN_ENTRY*(Is_unicode? 2 : 1 );
  }
  // go back to the short entry
  fs_g_nav_fast.u16_entry_pos_sel_file += u8_nb_entry;
  return TRUE;
}


//! This fonction create a long entry file in the current entry
//!
//! @param     sz_name      name to create (ASCII or UNICODE)
//! @param     u8_crc       crc corresponding at short name
//! @param     u8_id        long entry number (1 to n + FS_ENTRY_LFN_LAST)
//!
//! @verbatim
//! OUT: Update the entry in cache sector with the long name entry
//! @endverbatim
//!
void  fat_create_long_name_entry( FS_STRING sz_name , U8 u8_crc , U8 u8_id  )
{
   PTR_CACHE ptr_entry;
   Bool b_end_of_name = FALSE;
  
   fat_cache_sector_is_modify();
   ptr_entry = fat_get_ptr_entry();
   *ptr_entry = u8_id;
   ptr_entry++; // The long name start at offset 1 of the entry file

   for( u8_id=1; u8_id<FS_SIZE_FILE_ENTRY ; u8_id++ , ptr_entry++ )
   {
      // fields with no characters
      if( 11 == u8_id)
      {
         *ptr_entry = FS_ATTR_LFN_ENTRY;  // attribut field
         continue;
      }
      if( (12 == u8_id)
      ||  (26 == u8_id)
      ||  (27 == u8_id) )
      {
         // Reserved field
         // *ptr_entry = 0x00; // Not necessary because the cache must be clean
         continue;
      }
      if( 13 == u8_id)
      {
         *ptr_entry = u8_crc;          // CRC field
         continue;
      }

      // fields with characters
      if( !b_end_of_name )
      {
         U16 u16_tmp;
         if( Is_unicode )
         {
            u16_tmp = ((FS_STR_UNICODE)sz_name)[0];
         }else{
            u16_tmp = sz_name[0];
         }
         if(('\\' == u16_tmp )
         || ('/'  == u16_tmp ) )
         {  // end of name
            u16_tmp = 0;   // set string flag end of name
         }
         if( 0 == u16_tmp )
         {
            b_end_of_name = TRUE;
         }
         *ptr_entry = LSB(u16_tmp);
         ptr_entry++;
         *ptr_entry = MSB(u16_tmp);
         u8_id++;
         sz_name += (Is_unicode? 2 : 1 );
      }
      else
      {  // end of name
         *ptr_entry = 0xFF;         // Padding mandatory
      }
   } // end of loop in long entry
}


//! This fonction create a short name entry in the cache sector OR in a string
//!
//! @param     sz_name        name to create (ASCII or UNICODE)
//! @param     short_name     short name format entry ( = 8+3 Bytes) (used only if mode = TRUE)
//! @param     nb             number add at short name (ex: name~nb)
//! @param     mode           TRUE  to give the short name <br>
//!                           FALSE to create a entry in the entry in cache sector
//!
//! @return    the CRC of the short name
//!
U8    fat_create_short_entry_name( FS_STRING sz_name , FS_STRING short_name , U8 nb , Bool mode  )
{
   PTR_CACHE ptr_entry = 0;
   U8 u8_i, u8_step, character;
   U8 crc;

   if( !mode )
   {
      // Modify entry to create short entry in the current entry 
      fat_cache_sector_is_modify();
      // Get entry to store the short name
      ptr_entry = fat_get_ptr_entry();
   }

   crc = u8_step = u8_i = 0;
   while( 1 )
   {
      if( Is_unicode )
      {
         character = ((FS_STR_UNICODE)sz_name)[0];
      }else{
         character = sz_name[0];
      }

      if( 0 == u8_step )
      {  // step 1 = translate the name
         if( ((FS_SIZE_SFNAME_WITHOUT_EXT-2) == u8_i)    // name field is full (-2 for "~1")
         ||  ('.'    == character)                       // is the end of name without extension
         ||  fat_check_eof_name(character)            )  // is the end of name
         {
            u8_step++;                                   // go to next step
            continue;
         }
      }
      if( 5 == u8_step )
      {  // step 6 = translate the extension
         if( (u8_i == FS_SIZE_SFNAME)                    // name field is full
         ||  fat_check_eof_name(character)            )  // is the end of name
         {
            u8_step++;                                   // go to next step
            continue;
         }
      }
      if( (0==u8_step) || (5==u8_step) )
      {  // steps to translate name
         character = fat_translate_char_shortname( character );
         sz_name += (Is_unicode? 2 : 1 );
         if( 0 == character )
         {
            continue;                                    // Bad character, ignore this one
         }
      }
      if( 4 == u8_step )
      {  // step 5 = find character '.'
         if( ('.'    == character)                       // is the end of name without extension
         ||  fat_check_eof_name(character)            )  // is the end of name
         {
            u8_step++;                                   // go to next step
         } else {
            sz_name += (Is_unicode? 2 : 1 );
         }
         continue;                                       // this step don't add a character in the short name
      }
      if( 3 == u8_step )
      {  // step 4 = add padding
         if( u8_i == FS_SIZE_SFNAME_WITHOUT_EXT )        // end of field name without extension
         {
            u8_step++;                                   // go to next step
            continue;
         }
         character = ' ';
      }
      if( 6 == u8_step )
      {  // step 7 = add padding in extension name
         if( u8_i == FS_SIZE_SFNAME )                    // end of field name with extension
         {
            break;                                       // end of loop while(1)
         }
         character = ' ';
      }
      if( 2 == u8_step )
      {  // step 3 = add the last caracter (it is a number)
         character = '0'+(nb%10);
         u8_step++;                                      // go to next step
      }
      if( 1 == u8_step )
      {  // step 2 = add character '~' or number
         if( nb<10 )
         {
            character = '~';
         }else{
            character = '0'+(nb/10);
         }
         u8_step++;                                      // go to next step
      }
      
      if( mode )
      {
         // Record the short name in buffer
         *short_name = character;
         short_name++;
      }else{
         // Record the character in short entry file
         *ptr_entry = character;
         ptr_entry++;
      }
      u8_i++;
      
      // Compute the CRC of the short name
      crc = (crc >> 1) + ((crc & 1) << 7);               // rotate
      crc += character;                                  // add next char
   } // End of loop while
   return crc;
}


//! This fonction research a unique short name
//!
//! @param     sz_name     long name to use
//!
//! @return the number used for the short name
//!
U8    fat_find_short_entry_name( FS_STRING sz_name  )
{
   U8 u8_nb = 0;
   U8 _MEM_TYPE_SLOW_ short_name[11];

   u8_nb = 0; // Init to first shprt name

   while(1)
   {
      u8_nb++; // Try next short name
      // Compute the short name
      fat_create_short_entry_name( sz_name , short_name , u8_nb , TRUE  );
      // Go to beginning of directory
      fs_g_nav_fast.u16_entry_pos_sel_file = 0;
      // Scan directory to find a short entry
      while(1)
      {
         // Read directory
         if ( !fat_read_dir())
         {
            if( FS_ERR_OUT_LIST == fs_g_status )
               return u8_nb;  // short name not exist, then good number
            else
               return 1; // Error but ignore
         }
         // check entry
         if( fat_entry_shortname_compare( short_name ) )
            break; // short name exist
         if( FS_ERR_ENTRY_EMPTY == fs_g_status )
            return u8_nb;  // short name not exist, then good number
         // Go to next entry
         fs_g_nav_fast.u16_entry_pos_sel_file++;
      }
   }
}


//! This fonction compare the short name with current entry
//!
//! @param     short_name     short name format entry (= 8+3 Bytes)
//!
//! @return    TRUE it is the same
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//!
Bool  fat_entry_shortname_compare( FS_STRING short_name )
{
   PTR_CACHE ptr_entry;

   ptr_entry = fat_get_ptr_entry();
   if( FS_ENTRY_END == *ptr_entry )             // end of directory
   {
      fs_g_status = FS_ERR_ENTRY_EMPTY;
      return FALSE;
   }
   if( (FS_ENTRY_DEL == *ptr_entry )            // entry deleted
   ||  (FS_ATTR_LFN_ENTRY == ptr_entry[11]) )   // long file name
   {
      fs_g_status = FS_ERR_ENTRY_BAD;
      return FALSE;
   }
   fs_g_status = FS_ERR_ENTRY_BAD;  // by default this entry is different then bad
   return (0==memcmp_ram2ram(ptr_entry , short_name , 8+3 ));
}

//! Table of characters no supported in a name
_CONST_TYPE_ U8 fs_s_tab_incorrect_char[]={':','*','?','"','<','>','|'};

//! This fonction check the character of name AND computed the number of entry file for the name
//!
//! @param     sz_name     long name to create
//!
//! @return    number of entry file necessary incluing the short name entry <br>
//!            if a caracter isn't supported then 0.
//!
U8    fat_check_name( FS_STRING sz_name  )
{
   U8 u8_nb_entry, u8_i, u8_j;
   U16 u16_character;

   // Compute the number of entry for this name
   u8_nb_entry = 2;     // a short entry + one long name entry minimum
   u8_i = FS_SIZE_LFN_ENTRY;
   while( 1 )
   {
      if( Is_unicode )
      {
         u16_character = ((FS_STR_UNICODE)sz_name)[0];
      }else{
         u16_character = sz_name[0];
      }
      if( fat_check_eof_name( u16_character ) )
         break;

      for( u8_j = 0 ; u8_j < sizeof(fs_s_tab_incorrect_char) ; u8_j++ )
      {
         if( u16_character == fs_s_tab_incorrect_char[u8_j] )
            return 0;   // incorrect character
      }
      if( 0 == u8_i )
      {
         u8_nb_entry++;
         u8_i = FS_SIZE_LFN_ENTRY;
      }
      u8_i--;
      sz_name += (Is_unicode? 2 : 1 );
   }
   return u8_nb_entry;
}

//! Table of character no supported in short name
_CONST_TYPE_ U8 fs_s_execption_char[]={'+',',','.',';','=','[',']'};

//! This fonction translate the character to short name character autorized
//!
//! @param     character   character to translate
//!
//! @return    the character translated <br>
//!            if no supported then 0
//!
U8    fat_translate_char_shortname( U8 character )
{
   U8 u8_j;
   
   if( (character<=' ') || ('~'<character) )
      return 0;
   if( ('a'<=character) && (character<='z') )
   {
      return (character - ('a'-'A')); // Change to upper case
   }
   for( u8_j = 0 ; u8_j < sizeof(fs_s_execption_char) ; u8_j++ )
   {
      if( character == fs_s_execption_char[u8_j] )
         return 0;
   }
   return character;       // write the character
}


//! This fonction alloc the number of entry in directory
//!
//! @param     u8_nb_entry    number of entry to alloc
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! OUT: Initialise the system on the last alloc free entry
//! @endverbatim
//!
Bool  fat_alloc_entry_free( U8 u8_nb_entry )
{
   PTR_CACHE ptr_entry;

   // start at the beginning of dir
   fs_g_nav_fast.u16_entry_pos_sel_file=0;   // go to beginning of directory
   // loop in directory
   while( 1 )
   {
      // Read current sector directory
      if( !fat_read_dir() )
      {
         if( FS_ERR_OUT_LIST != fs_g_status )
            return FALSE;

         // The position is at out off the cluster list
            
         // Alloc a new sector (= new cluster)
         // Remark: The fs_g_seg.u32_addr containt the last cluster value of a directory list to link with the new list
         fs_g_seg.u32_size_or_pos = 1;
         if( !fat_allocfreespace())
            return FALSE;
         
         // Clean this new cluster
         // Remark: The fs_g_seg.u32_addr containt the new cluster value
         if( !fat_clear_cluster())
            return FALSE;
            
         continue;  // Rescan the directory list for found the new sector
      }

      // Check entry
      ptr_entry = fat_get_ptr_entry();
      if ( FS_ENTRY_END == *ptr_entry )
      {  // the entry is free
         u8_nb_entry--;
         if( 0 == u8_nb_entry )
         {
            return TRUE;  // All free entry is alloc
         }
      }
         
      // go to next entry
      fs_g_nav_fast.u16_entry_pos_sel_file++;
   }  // end of while(1)
}


//! This fonction delete the file entrys and cluster list
//!
//! @param     b_cluster_list    TRUE,  delete entrys file and cluster list
//!                              FALSE, delete only entrys file
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_delete_file( Bool b_cluster_list )
{
   PTR_CACHE ptr_entry;
   U8 u8_tmp;
   Bool b_short_del = FALSE;

   // loop in directory
   while( 1 )
   {
      // Read current sector directory
      if( !fat_read_dir() )
         return FALSE;

      // Get entry
      ptr_entry = fat_get_ptr_entry();
      u8_tmp = ptr_entry[0];

      if( (FS_ATTR_LFN_ENTRY != ptr_entry[11])
      &&  (b_short_del) )
      {
         // Only a short entry to del, no long entry exist
         break;   // Go to del cluster list
      }

      // Delete entry
      b_short_del = TRUE;
      ptr_entry[0] = FS_ENTRY_DEL;
      fat_cache_sector_is_modify();

      if( (FS_ATTR_LFN_ENTRY == ptr_entry[11])        // a entry long name
      &&  ( 0 != (FS_ENTRY_LFN_LAST & u8_tmp)) )      // last entry long name
      {
         break;   // Go to del cluster list
      }

      // go to previous entry
      fs_g_nav_fast.u16_entry_pos_sel_file--;
   }  // end of while(1)

   if( b_cluster_list )
   {
      // Delete cluster list
      fs_g_nav_entry.u32_pos_in_file=0;   // Del ALL list (start at begining)
      if( !fat_read_file( FS_CLUST_ACT_CLR ))
         return FALSE;
   }

   return TRUE;
}
#endif  // FS_LEVEL_FEATURES



//! This fonction return the space free in the current drive partition
//!
//! @return    the number of sector free <br>
//!            if 0, then error or full
//!
U32   fat_getfreespace( void )
{
   U32 u32_nb_free_cluster = 0;

   // Read ALL FAT1
   fs_g_cluster.u32_pos = 2;

   if( Is_fat12 )
   {  // FAT12 only
      for(
      ;     fs_g_cluster.u32_pos < fs_g_nav.u32_CountofCluster
      ;     fs_g_cluster.u32_pos++ )
      {
         // Get the value of the cluster
         if ( !fat_cluster_val( FS_CLUST_VAL_READ ) )
            return 0;
   
         if ( 0 == fs_g_cluster.u32_val )
            u32_nb_free_cluster++;
      }
   }
   else
   {
      if( Is_fat32 )
      {
         u32_nb_free_cluster = fat_read_fat32_FSInfo();
         if( 0xFFFFFFFF != u32_nb_free_cluster )
            goto endof_fat_getfreespace;      
         u32_nb_free_cluster = 0;
      }
      // Speed optimization only for FAT16 and FAT32
      // init first value used by fat_cluster_readnext()
      if( !fat_cluster_val( FS_CLUST_VAL_READ ))
         return FALSE;
      for(
      ;     fs_g_cluster.u32_pos < fs_g_nav.u32_CountofCluster
      ;     fs_g_cluster.u32_pos++ )
      {
         if ( 0 == fs_g_cluster.u32_val )
            u32_nb_free_cluster++;
         if( !fat_cluster_readnext() )
            return FALSE;
      }
      if( Is_fat32 )
      {
         // Save value for next call
         fat_write_fat32_FSInfo( u32_nb_free_cluster );
      }
   }
endof_fat_getfreespace:
   return (u32_nb_free_cluster * fs_g_nav.u8_BPB_SecPerClus);
}


//! This fonction return the space free in percent
//!
//! @return    percent of free space (1 to 100)
//!            if 0, then error or full
//!
//! @verbatim
//! More speed than fat_getfreespace() routine but delta error 1%
//! @endverbatim
//!
U8    fat_getfreespace_percent( void )
{
   U32 u32_nb_free_cluster = 0;
   U16 u16_tmp, u16_pos;

   if( Is_fat12 )
   {  // No speed optimization necessary on FAT12
      return (((fat_getfreespace()/fs_g_nav.u8_BPB_SecPerClus)*100) / fs_g_nav.u32_CountofCluster);
   }
   

   fs_g_cluster.u32_pos = 2;
   // init first value used by fat_cluster_readnext()
   if( !fat_cluster_val( FS_CLUST_VAL_READ ))
      return FALSE;

   // The optimization is to 
   // - read only the LSB byte of cluster
   // - read only 1 cluster for 2 clusters
   if( Is_fat32 )
   {  
      u32_nb_free_cluster = fat_read_fat32_FSInfo();
      if( 0xFFFFFFFF != u32_nb_free_cluster )
         goto endof_fat_getfreespace_percent;
      u32_nb_free_cluster = 0;
      
      u16_pos = 2*4;
      for(  u16_tmp = fs_g_nav.u16_fat_size
      ;     u16_tmp!=0
      ;     u16_tmp-- )
      {
         for( ; u16_pos < 512 ; u16_pos += (2*4) )
         {
            if( 0 == fs_g_sector[u16_pos] )
               u32_nb_free_cluster+=2;
         }         
         // Read next sector in FAT
         u16_pos = 0;
         fs_gu32_addrsector++;
         if( !fat_cache_read_sector( TRUE ))
            return 0;
      }
   }

   if ( Is_fat16 )
   {  
      u16_pos = 2*2;

      for(  u16_tmp = fs_g_nav.u16_fat_size
      ;     u16_tmp!=0
      ;     u16_tmp-- )
      {
         for( ; u16_pos < 512 ; u16_pos += (2*2) )
         {
            if( 0 == fs_g_sector[u16_pos] )
               u32_nb_free_cluster+=2;
         }         
         // Read next sector in FAT
         u16_pos = 0;
         fs_gu32_addrsector++;
         if( !fat_cache_read_sector( TRUE ))
            return 0;
      }
   }

   // Compute percent
   if( u32_nb_free_cluster > fs_g_nav.u32_CountofCluster )
      return 100;
   if( u32_nb_free_cluster > ((fs_g_nav.u32_CountofCluster-u32_nb_free_cluster)/256) )
   {
      // Compute and add a delta error
      u32_nb_free_cluster -= ((fs_g_nav.u32_CountofCluster-u32_nb_free_cluster)/256);
   }
endof_fat_getfreespace_percent:
   return ((u32_nb_free_cluster * 100) / fs_g_nav.u32_CountofCluster);
}



#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
//! This fonction alloc a continue segment in free space
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Global variables used
//! IN : 
//!   fs_g_seg.u32_addr          Last cluster value of a other list to link with the new list
//!                              If no link then set MSB0(fs_g_seg.u32_addr) to 0xFF
//!   fs_g_seg.u32_size_or_pos   Maximum size of segment alloc (unit sector)
//! OUT:
//!   fs_g_seg.u32_addr          Return the first cluster value of the new list
//!   fs_g_seg.u32_size_or_pos   The number of sector remainning
//! @endverbatim
//!
Bool  fat_allocfreespace( void )
{
   Bool first_cluster_free_is_found = FALSE; // Flag to signal the first step
   Bool b_quick_find = TRUE;  // Flag to signal the first step

   if( Is_fat32 )
   {
      // Clear info about free space
      if( !fat_write_fat32_FSInfo( 0xFFFFFFFF ))
         return FALSE;
   }
   
   if( 0xFF == MSB0(fs_g_seg.u32_addr) )
   {
fat_allocfreespace_start:
      // New cluster list, then research at the beginning
      fs_g_cluster.u32_pos = 2;
   }else{
      // increment a list then start at the next cluster of end of list
      fs_g_cluster.u32_pos = fs_g_seg.u32_addr+1;
   }
      
   fat_clear_info_fat_mod();
   
   // Read ALL FAT1
   for(  
   ;     fs_g_cluster.u32_pos < fs_g_nav.u32_CountofCluster
   ;     fs_g_cluster.u32_pos++ )
   {
      // Get the value of the cluster
      if ( !fat_cluster_val( FS_CLUST_VAL_READ ) )
         return FALSE;

      if ( 0 == fs_g_cluster.u32_val )
      {
         // Link the new cluster with previous cluster 
         fs_g_cluster.u32_val = fs_g_cluster.u32_pos; // value of the cluster is the new free cluster
         if( TRUE == first_cluster_free_is_found )
         {
            fs_g_cluster.u32_pos--;                      // select the previous cluster
            if ( !fat_cluster_val( FS_CLUST_VAL_WRITE ) )
               return FALSE;
         }
         else
         {
            // It is the first cluster of the new list
            first_cluster_free_is_found = TRUE;
            
            if( 0xFF != MSB0(fs_g_seg.u32_addr) )
            {
               // Link this new list with other cluster list
               // select the last cluster of the previous list
               if( 0 == fs_g_seg.u32_addr )
               {  // The previous list is the first cluster of root dir
                  if( FS_TYPE_FAT_32 != fs_g_nav_fast.u8_type_fat )
                  {
                     break; // Impossible to increment ROOT DIR size of FAT12 or FAT16
                  }
                  fs_g_cluster.u32_pos = fs_g_nav.rootdir.u32_cluster;
               }
               else
               {
                  fs_g_cluster.u32_pos = fs_g_seg.u32_addr;
               }
               if ( !fat_cluster_val( FS_CLUST_VAL_WRITE ) )
                  return FALSE;
            }  // else no writing the first cluster value in FAT because no previous cluster
            fs_g_seg.u32_addr = fs_g_cluster.u32_val;    // save the first cluster value
         }
            
         // Write in new cluster the end of list
         fs_g_cluster.u32_pos = fs_g_cluster.u32_val; // select the new cluster
         fs_g_cluster.u32_val = FS_CLUST_VAL_EOL;     // value of the cluster is flag end of list
         if ( !fat_cluster_val( FS_CLUST_VAL_WRITE ) )
            return FALSE;
         
         // Compute the remaining of sector
         if ( fs_g_seg.u32_size_or_pos <= fs_g_nav.u8_BPB_SecPerClus )
         {
            fs_g_seg.u32_size_or_pos = 0; // All space found 
            break; // stop loop
         }
         fs_g_seg.u32_size_or_pos -= fs_g_nav.u8_BPB_SecPerClus;
      }
      else
      {
         if( TRUE == first_cluster_free_is_found )
         {
            // End of continue segment free
            break; // stop loop
         }
         else
         {
            // It is the first step (find the first free cluster)
            // ignore this cluster no free
            if( b_quick_find )
            {
               fs_g_cluster.u32_pos += 500;
            }
         }
      }
   }
   
   // End of alloc
   if( FALSE == first_cluster_free_is_found )
   {
      if( b_quick_find )
      {  
         // Retry in no quick mode
         b_quick_find = FALSE;
         goto fat_allocfreespace_start;
      }
      fs_g_status = FS_ERR_NO_FREE_SPACE; // NO FREE CLUSTER FIND
      return FALSE;
   }

   return fat_update_fat2();
}
#endif  // FS_LEVEL_FEATURES


#if (FS_LEVEL_FEATURES > FSFEATURE_READ)
//! This fonction clear the informations about FAT modification
//!
void  fat_clear_info_fat_mod( void )
{
   fs_g_u16_first_mod_fat = 0xFFFF;
   fs_g_u16_last_mod_fat = 0;
}
#endif  // FS_LEVEL_FEATURES


#if (FS_LEVEL_FEATURES > FSFEATURE_READ)
//! This fonction copy the modification of first FAT to second FAT
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
Bool  fat_update_fat2( void )
{
  // Computed logical sector address
  while( fs_g_u16_first_mod_fat <= fs_g_u16_last_mod_fat )
  {
     // Compute the position about modification of FAT 1
     fs_gu32_addrsector = fs_g_nav.u32_ptr_fat + fs_g_u16_first_mod_fat;
     // Read FAT1
      if( !fat_cache_read_sector( TRUE ))
         return FALSE;
     // Compute the position about modification of FAT 2
     fs_gu32_addrsector = fs_g_nav.u32_ptr_fat + ((U32)fs_g_u16_first_mod_fat + fs_g_nav.u16_fat_size);
     // Init the sector FAT2 with the previous sector of the FAT1
     if( !fat_cache_read_sector( FALSE ))
         return FALSE;
     // Flag, The sector FAT2 is modify
     fat_cache_sector_is_modify();
     fs_g_u16_first_mod_fat++;
  }
  return TRUE;
}
#endif  // FS_LEVEL_FEATURES


#if (FSFEATURE_WRITE == (FS_LEVEL_FEATURES & FSFEATURE_WRITE))
//! This fonction clear a cluster
//!
//! @return    FALSE in case of error, see global value "fs_g_status" for more detail
//! @return    TRUE otherwise
//!
//! @verbatim
//! Global variables used
//! IN : 
//!   fs_g_seg.u32_addr          Cluster value to clear
//! @endverbatim
//!
Bool  fat_clear_cluster( void )
{
   U8 u8_loop;

   // Compute the address sector of cluster
   fs_g_seg.u32_size_or_pos  = 0; // Select the beginning of cluster
   if( !fat_cluster_list( FS_CLUST_ACT_ONE ))
      return FALSE;
   
   // Loop in the cluster (start at the last sector of cluster)
   fs_gu32_addrsector = fs_g_seg.u32_addr + (fs_g_nav.u8_BPB_SecPerClus -1);
   for(  u8_loop = 0
   ;     fs_g_nav.u8_BPB_SecPerClus != u8_loop
   ;     u8_loop++ )
   {
      if( !fat_cache_read_sector( FALSE ))
         return FALSE;
      
      if(0 == u8_loop)
      {  // Clean buffer only for the first shot
         fat_cache_clear();
      }
      fat_cache_sector_is_modify();
      fs_gu32_addrsector--;   // go to previous sector
   }
   return TRUE;
}
#endif  // FS_LEVEL_FEATURES


