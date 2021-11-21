/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief PDCA driver for AVR32 UC3.
 *
 * This file defines a useful set of functions for the PDCA interface on AVR32
 * devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with PDCA and USART modules.
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


#include "pdca.h"


volatile avr32_pdca_channel_t *pdca_get_handler(int pdca_ch_number)
{
  volatile avr32_pdca_t* pdca = (volatile avr32_pdca_t*) &AVR32_PDCA;
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) &pdca->channel[pdca_ch_number]; // get the correct channel pointer

  if( (pdca_ch_number >= AVR32_PDCA_CHANNEL_LENGTH) )
    return (volatile avr32_pdca_channel_t *) PDCA_INVALID_ARGUMENT;

  return (volatile avr32_pdca_channel_t*) pdca_channel;
}


int pdca_init_channel(int pdca_ch_number, const pdca_channel_options_t *opt)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_disable_interrupt_transfer_complete(pdca_ch_number); // disable channel interrupt
  pdca_disable_interrupt_reload_counter_zero(pdca_ch_number); // disable channel interrupt

  pdca_channel->mar= opt->addr;
  pdca_channel->tcr = opt->size;
  pdca_channel->psr = opt->pid;
  pdca_channel->marr = opt->r_addr;
  pdca_channel->tcrr = opt->r_size;
  pdca_channel->mr = opt->mode;

  return PDCA_SUCCESS;
}


void pdca_disable(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  // Disable transfer
  pdca_channel->cr = AVR32_PDCA_TDIS_MASK;

}


void pdca_enable(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  // Enable transfer
  pdca_channel->cr = AVR32_PDCA_TEN_MASK;
}


int pdca_get_channel_status(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  return pdca_channel->SR.ten ;
}


void pdca_set_memory_add(int pdca_ch_number, unsigned long mem_add, int transfer_counter)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->mar= mem_add;
  pdca_channel->tcr = transfer_counter;
}


void pdca_set_peripheral_select(int pdca_ch_number, int pid)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->psr = pid;
}


void pdca_set_mode(int pdca_ch_number, int mode)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->mr = mode;
}


void pdca_enable_interrupt_transfer_complete(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->ier = AVR32_PDCA_TRC_MASK;
}


void pdca_disable_interrupt_transfer_complete(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->idr = AVR32_PDCA_TRC_MASK;
}


void pdca_enable_interrupt_reload_counter_zero(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->ier = AVR32_PDCA_RCZ_MASK;
}


void pdca_disable_interrupt_reload_counter_zero(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  pdca_channel->idr = AVR32_PDCA_RCZ_MASK;
}


unsigned int pdca_get_transfer_status(int pdca_ch_number)
{
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number); // get the correct channel pointer

  return pdca_channel->isr;
}


void pdca_reload_channel(int pdca_ch_number, unsigned int address, unsigned int size)
{
  // get the correct channel pointer
  volatile avr32_pdca_channel_t* pdca_channel =(volatile avr32_pdca_channel_t*) pdca_get_handler(pdca_ch_number);

  // disable channel interrupt
  pdca_disable_interrupt_reload_counter_zero(pdca_ch_number);
  // set up next memory address
  pdca_channel->marr = address;
  // set up next memory size
  pdca_channel->tcrr = size;
  // Enable channel interrupt
  pdca_enable_interrupt_reload_counter_zero(pdca_ch_number);
}
