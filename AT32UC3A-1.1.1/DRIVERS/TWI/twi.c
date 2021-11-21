/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief TWI driver for AVR32 UC3.
 *
 * This file defines a useful set of functions for TWI on AVR32 devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a TWI module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

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
#include "intc.h"
#include "twi.h"


//! Pointer to the instance 1 of the TWI registers for IT.
static volatile avr32_twi_t *twi_inst1 = &AVR32_TWI;

//! Pointer to the applicative TWI transmit buffer.
static char *twi_tx_data = NULL;
//! Pointer to the applicative TWI receive buffer.
static char *twi_rx_data = NULL;

//! Remaining number of bytes to transmit.
static int twi_tx_nb_bytes = 0;
//! Remaining number of bytes to receive.
static int twi_rx_nb_bytes = 0;

//! IT mask.
static unsigned long twi_it_mask;

/*! \brief TWI interrupt handler.
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void twi_inst1_interrupt_handler(void)
{
  // get masked status register value
  int status = twi_inst1->sr & twi_it_mask;

  // this is a NACK
  if (status & AVR32_TWI_SR_NACK_MASK)
  {
    goto nack;
  }
  // this is a RXRDY
  else if (status & AVR32_TWI_SR_RXRDY_MASK)
  {
    // get data from Receive Holding Register
    *twi_rx_data++ = twi_inst1->rhr;
    // last byte to receive
    if(--twi_rx_nb_bytes==1)
    {
      // set stop bit
      twi_inst1->cr = AVR32_TWI_STOP_MASK;
    }
    // receive complete
    if (twi_rx_nb_bytes==0)
    {
      // finish the receive operation
      goto complete;
    }
  }
  // this is a TXRDY
  else if (status & AVR32_TWI_SR_TXRDY_MASK)
  {
    // decrease transmited bytes number
    twi_tx_nb_bytes--;
    // no more bytes to transmit
    if (twi_tx_nb_bytes <= 0)
    {
      // enable TXCOMP IT and unmask all others IT
      twi_it_mask = AVR32_TWI_IER_TXCOMP_MASK;
      twi_inst1->ier = twi_it_mask;
    }
    else
    {
      // put the byte in the Transmit Holding Register
      twi_inst1->thr = (unsigned long)*twi_tx_data++;
    }
  }
  // this is a TXCOMP
  else if (status & AVR32_TWI_SR_TXCOMP_MASK)
  {
    // finish the transmit operation
    goto complete;
  }

  return;

nack:
  // add some code here if you want to handle NACK operations
complete:
  // disable all interrupts
  twi_disable_interrupt(twi_inst1);

  return;
}



/*! \brief Set the twi bus speed in cojunction with the clock frequency
 *
 * \param twi    Base address of the TWI (i.e. &AVR32_TWI).
 * \param speed  The desired twi bus speed
 * \param pba_hz The current running PBA clock frequency
 * \return TWI_SUCCESS
 */
static int twi_set_speed(volatile avr32_twi_t *twi, unsigned int speed, unsigned long pba_hz)
{
  unsigned int cldiv;
  unsigned int ckdiv = 0;

  cldiv = (pba_hz / speed) - 4;

  // cldiv must fit in 8 bits, ckdiv must fit in 3 bits
  while ((cldiv > 0xFF) && (ckdiv < 0x7))
  {
    // increase clock divider
    ckdiv++;
    // divide cldiv value
    cldiv /= 2;
  }
  // set clock waveform generator register
  twi->cwgr = ( cldiv | (cldiv << AVR32_TWI_CWGR_CHDIV_OFFSET) | (ckdiv << AVR32_TWI_CWGR_CKDIV_OFFSET) );

  return TWI_SUCCESS;
}

int twi_init(volatile avr32_twi_t *twi, const twi_options_t *opt)
{
  int status;

  // Disable TWI interrupts
  twi->idr = ~0UL;

  // Reset TWI
  twi->cr = AVR32_TWI_CR_SWRST_MASK;

  // Dummy read in SR
  status = twi->sr;

  // Disable all interrupts
  Disable_global_interrupt();

  // Register TWI handler on level 2
  INTC_register_interrupt( &twi_inst1_interrupt_handler, AVR32_TWI_IRQ, INT2);

  // Enable all interrupts
  Enable_global_interrupt();

  // Select the speed
  twi_set_speed(twi, opt->speed, opt->pba_hz);

  // Probe the component
  status = twi_probe(twi, opt->chip) ;

  return status;
}

void twi_disable_interrupt(volatile avr32_twi_t *twi)
{
  twi->idr = ~0UL;
}

int twi_probe(volatile avr32_twi_t *twi, char chip_addr)
{
  twi_package_t package;
  char data[1] = {0};

  // data to send
  package.buffer = data;
  // chip address
  package.chip = chip_addr;
  // frame length
  package.length = 1;
  // address length
  package.addr_length = 0;
  // internal chip address
  package.addr = 0;
  // perform a master write access
  return (twi_master_write(twi, &package));
}

int twi_master_read(volatile avr32_twi_t *twi, const twi_package_t *package)
{
  int rx_timeout;

  // check argument
  if (package->length == 0)
  {
    return TWI_INVALID_ARGUMENT;
  }

  // set read mode, slave address and 3 internal address byte length
  twi->mmr = (package->chip << AVR32_TWI_MMR_DADR_OFFSET) |
             ((package->addr_length << AVR32_TWI_MMR_IADRSZ_OFFSET) & AVR32_TWI_MMR_IADRSZ_MASK) |
             (1 << AVR32_TWI_MMR_MREAD_OFFSET);

  // set internal address for remote chip
  twi->iadr = package->addr;

  // get a pointer to applicative data
  twi_rx_data = package->buffer;

  // get a copy of nb bytes to read
  twi_rx_nb_bytes = package->length;

  // Enable master transfer
  twi->cr =  AVR32_TWI_CR_MSEN_MASK;

  // Send start condition
  twi->cr = AVR32_TWI_START_MASK ;

  // only one byte to receive
  if(twi_rx_nb_bytes == 1)
  {
    // set stop bit
    twi->cr = AVR32_TWI_STOP_MASK;
  }    

  // mask NACK and RXRDY interrupts
  twi_it_mask = AVR32_TWI_IER_NACK_MASK | AVR32_TWI_IER_RXRDY_MASK;

  // update IMR through IER
  twi->ier = twi_it_mask;

  // setup timeout value
  rx_timeout = TWI_TIMEOUT * twi_rx_nb_bytes;

  // get data while timeout decrease
  while (twi_rx_nb_bytes && --rx_timeout);

  // Disable master transfer
  twi->cr =  AVR32_TWI_CR_MSDIS_MASK;

  // if timeout occurs
  if (rx_timeout == 0)
  {
    return TWI_TIMEOUT_ERR;
  }

  // everything is ok
  return TWI_SUCCESS;
}

int twi_master_write(volatile avr32_twi_t *twi, const twi_package_t *package)
{
  int tx_timeout;

  // No data to send
  if (package->length == 0)
  {
    return TWI_INVALID_ARGUMENT;
  }

  // Enable master transfer
  twi->cr =  AVR32_TWI_CR_MSEN_MASK;

  // set write mode, slave address and 3 internal address byte length
  twi->mmr = (0 << AVR32_TWI_MMR_MREAD_OFFSET) |
             (package->chip << AVR32_TWI_MMR_DADR_OFFSET) |
             ((package->addr_length << AVR32_TWI_MMR_IADRSZ_OFFSET) & AVR32_TWI_MMR_IADRSZ_MASK);

  // set internal address for remote chip
  twi->iadr = package->addr;

  // get a pointer to applicative data
  twi_tx_data = package->buffer;

  // get a copy of nb bytes to write
  twi_tx_nb_bytes = package->length;

  // put the first byte in the Transmit Holding Register
  twi_inst1->thr = (unsigned long)*twi_tx_data++;

  // mask NACK and TXRDY interrupts
  twi_it_mask = AVR32_TWI_IER_NACK_MASK | AVR32_TWI_IER_TXRDY_MASK;

  // update IMR through IER
  twi->ier = twi_it_mask;

  // setup timeout value
  tx_timeout = TWI_TIMEOUT * twi_tx_nb_bytes;

  // send data while timeout decrease
  while (twi_tx_nb_bytes && --tx_timeout);

  // Disable master transfer
  twi->cr =  AVR32_TWI_CR_MSDIS_MASK;

  // if timeout occurs
  if (tx_timeout == 0)
  {
    return TWI_TIMEOUT_ERR;
  }

  // Everything went ok
  return TWI_SUCCESS;
}
