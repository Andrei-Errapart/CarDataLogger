/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief Power Manager Example
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a Power Manager.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/*! \page License
 * Copyright (c) 2007, Atmel Corporation All rights reserved.
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

/*! \mainpage
* \section intro Introduction
* This is the documentation for the data structures, functions, variables, defines, enums, and
* typedefs for the power manager driver. <BR>It also gives an example of the usage of the
* PM on UC3 products.
* This example shows how to configure the Power Manager to use Oscillator 0 as source of main clock
* then configure a generic clock GCLK with this OSC0 as input. The generic clock 0 can be then viewed
* on the GCLK0 pin (this is the oscillator 0 frequency).
*
* \section compinfo Compilation Info
* This software was written for the GNU GCC for AVR32 and IAR Systems compiler
* for AVR32. Other compilers may or may not work.
*
* \section deviceinfo Device Info
* All AVR32 devices with a PM module can be used. This example has been tested
* with the following setup:<BR>
* <ul><li>EVK1100 evaluation kit</ul>
*
* \section setupinfo Setup Information
* <BR>CPU speed: <i> 12 MHz </i>
* - Check GCLK0 pin with an oscilloscope, this is OSC0 frequency (12MHz).
* On EVK1100, GCLK_0_1 is pin number 51 (PB19) with AT32UC3A0512 in QFP144 package.
*
* \section contactinfo Contact Info
* For more info about Atmel AVR32 visit<BR>
* <A href="http://www.atmel.com/products/AVR32/" >Atmel AVR32</A><BR>
* Support and FAQ: http://support.atmel.no/
*/


#include "board.h"
#include "gpio.h"
#include "pm.h"


/* Switch to clock Osc0 (crystal) */
static void local_switch_to_osc0(volatile avr32_pm_t* pm)
{
  pm_enable_osc0_crystal(pm, FOSC0);  // with Osc
  pm_enable_clk0(pm, OSC0_STARTUP);
  pm_switch_to_clock(pm, AVR32_PM_MCSEL_OSC0);
}


/* Enable GCLK0 output on AVR32_PM_GCLK_0_1_PIN*/
static void local_enable_gclk0_on_gpio(volatile avr32_pm_t* pm)
{
  int gc=0; /* gclk 0 is used */

  /* setup gc0 on Osc0, no divisor */
  pm_gc_setup(pm, gc, AVR32_GC_USES_OSC, AVR32_GC_USES_OSC0, AVR32_GC_NO_DIV_CLOCK, 0);

  /* Now enable the generic clock */
  pm_gc_enable(pm,gc);

  /* Assign a GPIO to generic clock GCLK0 output */
  gpio_enable_module_pin(AVR32_PM_GCLK_0_1_PIN, AVR32_PM_GCLK_0_1_FUNCTION);
  // Note that gclk0_1 is pin 51 pb19 on AT32UC3A0512 QFP144.
}


/* \brief This is an example of how to configure the Power Manager to use Oscillator 0 as main clock
 * then configure a generic clock GCLK to be outputed to GLCK_0_1.
 *
 */
int main(void)
{
  volatile avr32_pm_t* pm = &AVR32_PM;
  local_switch_to_osc0(pm);
  local_enable_gclk0_on_gpio(pm);

  while(1);
}
