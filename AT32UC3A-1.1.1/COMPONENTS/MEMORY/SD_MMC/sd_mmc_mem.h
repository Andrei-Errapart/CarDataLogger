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
#ifndef _SD_MMC_MEM_H_
#define _SD_MMC_MEM_H_

#include "conf_access.h"
#include "ctrl_access.h"



//_____ D E F I N I T I O N S ______________________________________________

#define   SD_MMC_REMOVED       0
#define   SD_MMC_INSERTED      1
#define   SD_MMC_REMOVING      2


//---- CONTROL FONCTIONS ----

// those fonctions are declared in sd_mmc_mem.h
extern void           sd_mmc_mem_init(void);
extern Ctrl_status    sd_mmc_test_unit_ready(void);
extern Ctrl_status    sd_mmc_read_capacity( U32 *u32_nb_sector );
extern Bool           sd_mmc_wr_protect(void);
extern Bool           sd_mmc_removal(void);


//---- ACCESS DATA FONCTIONS ----

// Standard functions for open in read/write mode the device
extern Ctrl_status    sd_mmc_read_10( U32 addr , U16 nb_sector );
extern Ctrl_status    sd_mmc_write_10( U32 addr , U16 nb_sector );

// Standard functions for open in read/write mode from HOST
extern Ctrl_status    sd_mmc_host_write_10( U32 addr , U16 nb_sector );
extern Ctrl_status    sd_mmc_host_read_10( U32 addr , U16 nb_sector );

// Standard functions for read/write 1 sector to 1 sector ram buffer
extern Ctrl_status    sd_mmc_ram_2_mem( U32 addr, U8 *ram);
extern Ctrl_status    sd_mmc_mem_2_ram( U32 addr, U8 *ram);

extern Ctrl_status    sd_mmc_ram_2_mem_write(void);
extern Ctrl_status    sd_mmc_mem_2_ram_read(void);


//** If your device transfer have a specific transfer for USB (Particularity of Chejudo product, or bootloader)
#ifdef SD_MMC_VALIDATION
#include "virtual_usb.h"
#else
#include "usb_drv.h"    // In this case the driver must be know the USB access
#endif
extern Ctrl_status sd_mmc_usb_read( void );
extern Ctrl_status sd_mmc_usb_write( void );


#endif   // _SD_MMC_MEM_H_
