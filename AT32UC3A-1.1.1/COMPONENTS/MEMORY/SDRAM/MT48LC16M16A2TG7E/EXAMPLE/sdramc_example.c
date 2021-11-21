/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SDRAMC on EBI example application.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an SDRAMC module can be used.
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


#include "board.h"
#include "print_funcs.h"
#include "pm.h"
#include "sdramc.h"


/*! \brief Sets the SDRAM Controller up, initializes the SDRAM found on the
 *         board and tests it.
 */
int main(void)
{
  unsigned long sdram_size, progress_inc, i, j, tmp, noErrors = 0;
  volatile unsigned long *sdram = SDRAM;

  // Switch to external oscillator 0.
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Initialize the debug USART module.
  init_dbg_rs232(FOSC0);

  // Calculate SDRAM size in words (32 bits).
  sdram_size = SDRAM_SIZE >> 2;
  print_dbg("SDRAM size: ");
  print_dbg_ulong(SDRAM_SIZE >> 20);
  print_dbg(" MB\n");

  // Initialize the external SDRAM chip.
  sdramc_init(FOSC0);
  print_dbg("SDRAM initialized\n");

  // Determine the increment of SDRAM word address requiring an update of the
  // printed progression status.
  progress_inc = (sdram_size + 50) / 100;

  // Fill the SDRAM with the test pattern.
  for (i = 0, j = 0; i < sdram_size; i++)
  {
    if (i == j * progress_inc)
    {
      LED_Toggle(LED_BI0_RED);
      print_dbg("\rFilling SDRAM with test pattern: ");
      print_dbg_ulong(j++);
      print_dbg_char('%');
    }
    sdram[i] = i;
  }
  LED_Off(LED_BI0_RED);
  print_dbg("\rSDRAM filled with test pattern       \n");

  // Recover the test pattern from the SDRAM and verify it.
  for (i = 0, j = 0; i < sdram_size; i++)
  {
    if (i == j * progress_inc)
    {
      LED_Toggle(LED_BI0_GREEN);
      print_dbg("\rRecovering test pattern from SDRAM: ");
      print_dbg_ulong(j++);
      print_dbg_char('%');
    }
    tmp = sdram[i];
    if (tmp != i)
    {
      noErrors++;
    }
  }
  LED_Off(LED_BI0_GREEN);
  LED_On((noErrors) ? LED_BI1_RED : LED_BI1_GREEN);
  print_dbg("\rSDRAM tested: ");
  print_dbg_ulong(noErrors);
  print_dbg(" corrupted word(s)       \n");

  while (1);
}
