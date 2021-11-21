/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief CTRL_ACCESS interface for SD/MMC card  .
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an SPI module can be used.
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

#if (SD_MMC_MEM == ENABLE)
//_____  I N C L U D E S ___________________________________________________
#include "compiler.h"
#include "conf_sd_mmc.h"
#include "sd_mmc_mem.h"
#include "sd_mmc.h"
#include "usb_drv.h"
#include "scsi_decoder.h"
#include "spi.h"
#include "conf_sd_mmc.h"

#include "board.h"
#include "gpio.h"
#include "spi.h"
//_____ M A C R O S ________________________________________________________
#define Sd_mmc_access_signal_on()
#define Sd_mmc_access_signal_off()

//_____ P R I V A T E    D E C L A R A T I O N _____________________________


//_____ D E F I N I T I O N ________________________________________________


//extern xdata U32 sd_mmc_mem_size;
extern            U32 SD_MMC_DISK_SIZE;
extern volatile   U32 sd_mmc_last_address;
extern            Bool sd_mmc_init_done;

U8                sd_mmc_presence_status = 0;
extern            Bool sd_mmc_init_done;
/*

*/
//_____ D E C L A R A T I O N ______________________________________________

//!
//! @brief This function initializes the hw/sw ressources required to drive the SD_MMC.
//!
//! @warning Code:?? bytes (function code length)
//!
//!/

void sd_mmc_mem_init(void)
{
  sd_mmc_restartinit();        // Restart Init of SD/MMC card after previous first init
}


//!
//! @brief This function tests the state of the SD_MMC memory and sends it to the Host.
//!        For a PC, this device is seen as a removable media
//!        Before indicating any modification of the status of the media (GOOD->NO_PRESENT or vice-versa),
//!         the function must return the BUSY data to make the PC accepting the change
//!
//! @warning Code:?? bytes (function code length)
//!
//! @return                Ctrl_status
//!   Media is ready       ->    CTRL_GOOD
//!   Media not present    ->    CTRL_NO_PRESENT
//!   Media has changed    ->    CTRL_BUSY
//!/
Ctrl_status sd_mmc_test_unit_ready(void)
{
  Sd_mmc_access_signal_on();
  switch (sd_mmc_presence_status)
  {
    case SD_MMC_REMOVED:
      sd_mmc_init_done = FALSE;
      if (OK == sd_mmc_mem_check())
      {
        sd_mmc_presence_status = SD_MMC_INSERTED;
        Sd_mmc_access_signal_off();
        return CTRL_BUSY;
      }
      Sd_mmc_access_signal_off();
      return CTRL_NO_PRESENT;

    case SD_MMC_INSERTED:
      if (OK != sd_mmc_mem_check())
      {
        sd_mmc_presence_status = SD_MMC_REMOVING;
        sd_mmc_init_done = FALSE;
        Sd_mmc_access_signal_off();
        return CTRL_BUSY;
      }
      Sd_mmc_access_signal_off();
      return CTRL_GOOD;

    case SD_MMC_REMOVING:
      sd_mmc_presence_status = SD_MMC_REMOVED;
      Sd_mmc_access_signal_off();
      return CTRL_NO_PRESENT;

    default:
      sd_mmc_presence_status = SD_MMC_REMOVED;
      Sd_mmc_access_signal_off();
      return CTRL_BUSY;
  }

  Sd_mmc_access_signal_off();
  return CTRL_BUSY;
/*
  if (OK==sd_mmc_mem_check())
  {
    if (sd_mmc_status_changed == FALSE)
    {
      sd_mmc_status_changed = TRUE;
      return CTRL_BUSY;     // BUSY token must be returned to indicate a status change !
    }
    else
      return CTRL_GOOD;     // the 2nd time the host will ask for unit_ready, we can answer GOOD if we have returned BUSY first !
  }
  else
  {
    if (sd_mmc_status_changed == TRUE)
    {
      sd_mmc_status_changed = FALSE;
      return CTRL_BUSY;     // BUSY token must be returned to indicate a status change !
    }
    else
      return CTRL_NO_PRESENT;
  }
*/
}


//!
//! @brief This function gives the address of the last valid sector.
//!
//! @warning Code:?? bytes (function code length)
//!
//! @param *u32_nb_sector  number of sector (sector = 512B). OUT
//!
//! @return                Ctrl_status
//!   Media ready          ->  CTRL_GOOD
//!   Media not present    ->  CTRL_NO_PRESENT
//!/
Ctrl_status sd_mmc_read_capacity( U32 *u32_nb_sector )
{
//   sd_mmc_check_presence();   // ommited because creates interferences with "sd_mmc_test_unit_ready()" function
   Sd_mmc_access_signal_on();

   if (sd_mmc_init_done == FALSE)
   {
      sd_mmc_mem_init();
   }

   if (sd_mmc_init_done == TRUE)
   {
     *u32_nb_sector = sd_mmc_last_block_address+1;
     Sd_mmc_access_signal_off();
     return CTRL_GOOD;
   }
   else
   {
     Sd_mmc_access_signal_off();
     return CTRL_NO_PRESENT;
   }
}


//!
//! @brief This function returns the write protected status of the memory.
//!
//! Only used by memory removal with a HARDWARE SPECIFIC write protected detection
//! !!! The customer must unplug the memory to change this write protected status,
//! which cannot be for a SD_MMC.
//!
//! @warning Code:?? bytes (function code length)
//!
//! @return FALSE  -> the memory is not write-protected (always)
//!/
Bool  sd_mmc_wr_protect(void)
{
   return FALSE;
}


//!
//! @brief This function tells if the memory has been removed or not.
//!
//! @warning Code:?? bytes (function code length)
//!
//! @return FALSE  -> The memory isn't removed
//!/
Bool  sd_mmc_removal(void)
{
  return FALSE;
//  return ((OK == sd_mmc_check_presence()) ? FALSE : TRUE);
}



//------------ STANDARD FUNCTIONS to read/write the memory --------------------

//!
//! @brief This function performs a read operation of n sectors from a given address on.
//! (sector = 512B)
//!
//!         DATA FLOW is: SD_MMC => USB
//!
//!
//! @warning Code:?? bytes (function code length)
//!
//! @param addr         Sector address to start the read from
//! @param nb_sector    Number of sectors to transfer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
Ctrl_status sd_mmc_read_10( U32 addr , U16 nb_sector )
{
Bool status;

   if (sd_mmc_init_done == FALSE)
   {
      sd_mmc_mem_init();
   }

   if (sd_mmc_init_done == TRUE)
   {
     Sd_mmc_access_signal_on();
     sd_mmc_read_open(addr);
     status = sd_mmc_read_multiple_sector(nb_sector);
     sd_mmc_read_close();
     Sd_mmc_access_signal_off();
     if (status == OK)
        return CTRL_GOOD;
     else
        return CTRL_NO_PRESENT;
   }
   else
     return CTRL_NO_PRESENT;
}


//! This fonction initialise the memory for a write operation
//!
//!         DATA FLOW is: USB => SD_MMC
//!
//!
//! (sector = 512B)
//! @param addr         Sector address to start write
//! @param nb_sector    Number of sectors to transfer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
Ctrl_status sd_mmc_write_10( U32 addr , U16 nb_sector )
{
  Bool status;

   if (sd_mmc_init_done == FALSE)
   {
      sd_mmc_mem_init();
   }

   if (sd_mmc_init_done == TRUE)
   {
     Sd_mmc_access_signal_on();
     sd_mmc_write_open(addr);
     status = sd_mmc_write_multiple_sector(nb_sector);
     sd_mmc_write_close();
     Sd_mmc_access_signal_off();
     if (status == OK)
       return CTRL_GOOD;
     else
       return CTRL_NO_PRESENT;
   }
   else
     return CTRL_NO_PRESENT;
}

void sd_mmc_read_multiple_sector_callback(const void *psector)
{
  U16 data_to_transfer = MMC_SECTOR_SIZE;

  while (data_to_transfer)
  {
    while (!Is_usb_in_ready(g_scsi_ep_ms_in));

    Usb_reset_endpoint_fifo_access(g_scsi_ep_ms_in);
    data_to_transfer = usb_write_ep_txpacket(g_scsi_ep_ms_in, psector,
                                             data_to_transfer, &psector);
    Usb_ack_in_ready_send(g_scsi_ep_ms_in);
  }
}

void sd_mmc_write_multiple_sector_callback(void *psector)
{
  U16 data_to_transfer = MMC_SECTOR_SIZE;

  while (data_to_transfer)
  {
    while (!Is_usb_out_received(g_scsi_ep_ms_out));

    Usb_reset_endpoint_fifo_access(g_scsi_ep_ms_out);
    data_to_transfer = usb_read_ep_rxpacket(g_scsi_ep_ms_out, psector,
                                            data_to_transfer, &psector);
    Usb_ack_out_received_free(g_scsi_ep_ms_out);
  }
}
//------------ SPECIFIC FONCTION USB TRANSFER -----------------------------------------

//** If your device transfer have a specific transfer for USB (Particularity of Chejudo product, or bootloader)
// !!! In this case the driver must be know the USB access

//! This fonction transfer the memory data (programed in scsi_read_10) directly to the usb interface
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!
Ctrl_status sd_mmc_usb_read()
{

   return CTRL_GOOD;
}


//! This fonction transfer the usb data (programed in scsi_write_10) directly to the memory data
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!
Ctrl_status sd_mmc_usb_write( void )
{

   return CTRL_GOOD;
}

//------------ FUNCTIONS FOR USED WITH USB HOST MODE (host mass storage)-----------------

//! This fonction initialise the memory for a write operation
//! in usb host mode
//!
//!         DATA FLOW is: SD_MMC => USB
//!
//! (sector = 512B)
//! @param addr         Sector address to start write
//! @param nb_sector    Number of sectors to transfer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
/*Ctrl_status sd_mmc_host_write_10( U32 addr , U16 nb_sector )
{
   sd_mmc_read_open(addr);
   sd_mmc_host_read_sector(nb_sector); //! Write_10 in host mode means reading the SD_MMC
   sd_mmc_read_close();
   return CTRL_GOOD;
} */

//! This fonction initialise the memory for a read operation
//! in usb host mode
//!
//!         DATA FLOW is: USB => SD_MMC
//!
//! (sector = 512B)
//! @param addr         Sector address to start write
//! @param nb_sector    Number of sectors to transfer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
/*Ctrl_status sd_mmc_host_read_10( U32 addr , U16 nb_sector )
{
   sd_mmc_write_open(addr);
   sd_mmc_host_write_sector(nb_sector); //! Read_10 in host mode means writing the SD_MMC
   sd_mmc_write_close();
   return CTRL_GOOD;
} */



//------------ Standard functions for read/write 1 sector to 1 sector ram buffer -----------------

//! This fonction initialise the memory for a write operation
//! from ram buffer to SD/MMC (1 sector)
//!
//!         DATA FLOW is: RAM => MMC
//!
//! (sector = 512B)
//! @param addr         Sector address to write
//! @param ram          Ram buffer pointer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
Ctrl_status    sd_mmc_ram_2_mem(U32 addr, U8 *ram)
{
#if (SD_MMC_RAM == ENABLE)
   Sd_mmc_access_signal_on();
   sd_mmc_check_presence();

   if (sd_mmc_init_done == FALSE)
   {
      sd_mmc_mem_init();
   }

   if (sd_mmc_init_done == TRUE)
   {
     sd_mmc_write_open(addr);
     if (KO == sd_mmc_write_sector_from_ram(ram))
     {
       sd_mmc_write_close();
       Sd_mmc_access_signal_off();
       return CTRL_NO_PRESENT;
     }
     sd_mmc_write_close();
     Sd_mmc_access_signal_off();
     return CTRL_GOOD;
   }
   Sd_mmc_access_signal_off();
#endif   // (SD_MMC_RAM == ENABLE)
     return CTRL_NO_PRESENT;
}

//! This fonction read 1 sector from SD/MMC to ram buffer
//!
//!         DATA FLOW is: MMC => RAM
//!
//! (sector = 512B)
//! @param addr         Sector address to read
//! @param ram          Ram buffer pointer
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!   A error occur  ->    CTRL_FAIL
//!
Ctrl_status    sd_mmc_mem_2_ram( U32 addr, U8 *ram)
{
#if (SD_MMC_RAM == ENABLE)
   Sd_mmc_access_signal_on();
   sd_mmc_check_presence();

   if (sd_mmc_init_done == FALSE)
   {
      sd_mmc_mem_init();
   }

   if (sd_mmc_init_done == TRUE)
   {
     sd_mmc_read_open(addr);
     if (KO == sd_mmc_read_sector_to_ram(ram))
     {
       sd_mmc_write_close();
       Sd_mmc_access_signal_off();
       return CTRL_NO_PRESENT;
     }
     sd_mmc_read_close();
     Sd_mmc_access_signal_off();
     return CTRL_GOOD;
   }
   Sd_mmc_access_signal_off();
#endif   // (SD_MMC_RAM == ENABLE)
   return CTRL_NO_PRESENT;
}



//! This fonction transfer the memory data to ram
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!
Ctrl_status sd_mmc_mem_2_ram_read()
{
   return CTRL_GOOD;
}


//! This fonction transfer ram to the memory data
//!
//! @return                Ctrl_status
//!   It is ready    ->    CTRL_GOOD
//!
Ctrl_status sd_mmc_ram_2_mem_write( void )
{
   return CTRL_GOOD;
}

#endif   //(SD_MMC_MEM == ENABLE)
