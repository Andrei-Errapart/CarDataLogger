/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief EIC driver for AVR32 UC3.
 *
 * AVR32 External Interrupt Controller driver module.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an EIC module can be used.
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
#include "preprocessor.h"
#include "eic.h"



void eic_init(volatile avr32_eic_t *eic, const eic_options_t *opt, const unsigned int nb_lines)
{
  int i;
  for (i=0;i<nb_lines;i++)
  {
    // Set up mode level
    eic->mode = ((opt[i].eic_mode)==1)?(eic->mode|(opt[i].eic_mode)<<(opt[i].eic_line)):(eic->mode&~((opt[i].eic_mode)<<(opt[i].eic_line)));
    // Set up edge type
    eic->edge = ((opt[i].eic_edge)==1)?(eic->edge|(opt[i].eic_edge)<<(opt[i].eic_line)):(eic->edge&~((opt[i].eic_edge)<<(opt[i].eic_line)));
    // Set up level
    eic->level = ((opt[i].eic_level)==1)?(eic->level|(opt[i].eic_level)<<(opt[i].eic_line)):(eic->level&~((opt[i].eic_level)<<(opt[i].eic_level)));
    // Set up if filter is used
    eic->filter = ((opt[i].eic_filter)==1)?(eic->filter|(opt[i].eic_filter)<<(opt[i].eic_line)):(eic->filter&~((opt[i].eic_mode)<<(opt[i].eic_line)));
    // Set up which mode is used : asynchronous mode/ synchronous mode
    eic->async = ((opt[i].eic_async)==1)?(eic->async|(opt[i].eic_async)<<(opt[i].eic_line)):(eic->async&~((opt[i].eic_mode)<<(opt[i].eic_line)));
  }
}

void eic_enable_lines(volatile avr32_eic_t *eic, const unsigned int mask_lines)
{
  eic->en = mask_lines;
}

void eic_enable_line(volatile avr32_eic_t *eic, const unsigned int line_number)
{
  // Enable line line_number
  eic->en = (1<<line_number);
}

void eic_disable_lines(volatile avr32_eic_t *eic, const unsigned int mask_lines)
{
  eic->dis = mask_lines;
}

void eic_disable_line(volatile avr32_eic_t *eic, const unsigned int line_number)
{
  // Disable line line_number
  eic->dis = (1<<line_number);
}

void eic_enable_interrupt_lines(volatile avr32_eic_t *eic, const unsigned int mask_lines)
{
  eic->ier = mask_lines;
}

void eic_enable_interrupt_line(volatile avr32_eic_t *eic, const unsigned int line_number)
{
  // Enable line line_number
  eic->ier = (1<<(line_number));
}

void eic_disable_interrupt_lines(volatile avr32_eic_t *eic, const unsigned int mask_lines)
{
  eic->idr = mask_lines;
}

void eic_disable_interrupt_line(volatile avr32_eic_t *eic, const unsigned int line_number)
{
  // Disable line line_number
  eic->idr = (1<<(line_number));
}

void eic_clear_interrupt_lines(volatile avr32_eic_t *eic, const unsigned int mask_lines)
{
  eic->icr = mask_lines;
}

void eic_clear_interrupt_line(volatile avr32_eic_t *eic, const unsigned int line_number)
{
  // Clear line line_number
  eic->icr = (1<<(line_number));
}

void eic_enable_interrupt_scan(volatile avr32_eic_t *eic,unsigned int presc)
{
  // Enable SCAN function with PRESC value
  eic->scan |= (presc<<AVR32_EIC_SCAN_PRESC_OFFSET)|(1<<AVR32_EIC_SCAN_EN_OFFSET);
}

void eic_disable_interrupt_scan(volatile avr32_eic_t *eic)
{
  // Disable SCAN function
  eic->scan = (0<<AVR32_EIC_SCAN_EN_OFFSET);
}

unsigned long eic_get_interrupt_pad_scan(volatile avr32_eic_t *eic)
{
  // Return pad number that causes interrupt
  return(eic->scan>>AVR32_EIC_SCAN_PIN_OFFSET);
}

