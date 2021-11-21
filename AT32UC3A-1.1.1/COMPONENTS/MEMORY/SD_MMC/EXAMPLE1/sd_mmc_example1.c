/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SD/MMC card driver example application.
 *
 * This file gives an example of using the SD/MMC card driver.
 * If a SD/MMC card is detected, a message is sent to user
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


#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "sd_mmc.h"
#include "usart.h"
#include "print_funcs.h"


/*! \brief PBA clock frequency (Hz) */
#define PBA_HZ                FOSC0

/*! \brief Number of bits in each SPI package*/
#define SPI_BITS              8

/*! \brief Number of SPI module instance to use */
#define SPI_MODULE_INSTANCE   &AVR32_SPI0

/*! \brief SPI master speed in Hz */
#define SPI_MASTER_SPEED      12000000

/*! \brief Number of bytes in the receive buffer when operating in slave mode */
#define BUFFERSIZE            64


/*! \brief Main function. Executing starts here.
 */
int main(void)
{
  U8 status;
  spi_options_t spiOptions =
  {
    .reg          = 1,
    .baudrate     = SPI_MASTER_SPEED, // Defined in conf_at45dbx.h
    .bits         = SPI_BITS,         // Defined in conf_at45dbx.h
    .spck_delay   = 0,
    .trans_delay  = 0,
    .stay_act     = 1,
    .spi_mode     = 0,
    .fdiv         = 0,
    .modfdis      = 1
  };

  /* Switch the main clock to the external oscillator 0 */
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
  init_dbg_rs232(PBA_HZ);
  print_dbg("INIT SD/MMC...");

  /* Initialize SPI as a Master */
  status = sd_mmc_init(spiOptions, PBA_HZ);
  status = sd_mmc_mem_check();
  if (status == OK )
  {
     print_dbg("Card detected!\n");
  }
  sd_mmc_get_capacity();
  while (1);
}
