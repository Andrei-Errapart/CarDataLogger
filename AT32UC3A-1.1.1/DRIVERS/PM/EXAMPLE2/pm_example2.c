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
 * <BR>This example shows:
 * - how to configure a PLL and switch the main clock to PLL output,
 * - configure a Generic Clock and output it to a GPIO pin,
 * - LED0 blinks endlessly.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a PM module can be used. This example has been tested
 * with the following setup:<BR>
 * <ul><li>EVK1100 evaluation kit
 * <li>EVK1101 evaluation kit</ul>
 *
 * \section setupinfo Setup Information
 * CPU speed: <i> 48 MHz </i>
 * - On \b EVK1100, check GCLK0 pin with an oscilloscope, the frequency should be 48MHz.
 * On EVK1100, GCLK_0 is pin number 51 (PB19) with AT32UC3A0512 in QFP144 package.
 * Since LED5 is also on PB19, you should also see LED5 turn red.
 * - On \b EVK1101, check GCLK2 pin with an oscilloscope, the frequency should be 48MHz.
 * On EVK1101, GCLK_2 is pin number 30 (PA30) with AT32UC3B0256 in QFP64 package.
 * - Check that LED0 blinks (whatever the board).
 *
 * \section contactinfo Contact Info
 * For more info about Atmel AVR32 visit<BR>
 * <A href="http://www.atmel.com/products/AVR32/" >Atmel AVR32</A><BR>
 * Support and FAQ: http://support.atmel.no/
 */


#include "gpio.h"
#include "flashc.h"
#include "pm.h"
#include "board.h"



/*! \name Generic Clock choice
 */
//! @{
#if BOARD == EVK1100
#  define EXAMPLE_GCLK_ID             0
#  define EXAMPLE_GCLK_PIN            AVR32_PM_GCLK_0_1_PIN
#  define EXAMPLE_GCLK_FUNCTION       AVR32_PM_GCLK_0_1_FUNCTION
// Note that gclk0_1 is pin 51 pb19 on AT32UC3A0512 QFP144.
#elif BOARD == EVK1101
#  define EXAMPLE_GCLK_ID             2
#  define EXAMPLE_GCLK_PIN            AVR32_PM_GCLK_2_PIN
#  define EXAMPLE_GCLK_FUNCTION       AVR32_PM_GCLK_2_FUNCTION
// Note that gclk_2 is pin 30 pa30 on AT32UC3B0256 QFP64.
#endif
//! @}


/* Start PLL0, enable a generic clock with PLL0 output then switch main clock to PLL0 output.
   All calculations in this function suppose that the Osc0 frequency is 12MHz. */
void local_start_pll0(volatile avr32_pm_t* pm)
{
  pm_switch_to_osc0(pm, FOSC0, OSC0_STARTUP);  // Switch main clock to Osc0.

  /* Setup PLL0 on Osc0, mul=3 ,no divisor, lockcount=16, ie. 12Mhzx8 = 96MHz output */
  /*void pm_pll_setup(volatile avr32_pm_t* pm,
                  unsigned int pll,
                  unsigned int mul,
                  unsigned int div,
                  unsigned int osc,
                  unsigned int lockcount) {
   */
  pm_pll_setup(pm,
               0,   // use PLL0
               7,   // MUL=7 in the formula
               1,   // DIV=1 in the formula
               0,   // Sel Osc0/PLL0 or Osc1/PLL1
               16); // lockcount in main clock for the PLL wait lock

  /*
   This function will set a PLL option.
   *pm Base address of the Power Manager (i.e. &AVR32_PM)
   pll PLL number 0
   pll_freq Set to 1 for VCO frequency range 80-180MHz, set to 0 for VCO frequency range 160-240Mhz.
   pll_div2 Divide the PLL output frequency by 2 (this settings does not change the FVCO value)
   pll_wbwdisable 1 Disable the Wide-Bandith Mode (Wide-Bandwith mode allow a faster startup time and out-of-lock time). 0 to enable the Wide-Bandith Mode.
  */
  /* PLL output VCO frequency is 96MHz. We divide it by 2 with the pll_div2=1. This enable to get later main clock to 48MHz */
  pm_pll_set_option(pm, 0, 1, 1, 0);

  /* Enable PLL0 */
  /*
    void pm_pll_enable(volatile avr32_pm_t* pm,
                  unsigned int pll) {
  */
  pm_pll_enable(pm,0);

  /* Wait for PLL0 locked */
  pm_wait_for_pll0_locked(pm) ;

  /* Setup generic clock on PLL0, with Osc0/PLL0, no divisor */
  /*
  void pm_gc_setup(volatile avr32_pm_t* pm,
                  unsigned int gc,
                  unsigned int osc_or_pll, // Use Osc (=0) or PLL (=1)
                  unsigned int pll_osc, // Sel Osc0/PLL0 or Osc1/PLL1
                  unsigned int diven,
                  unsigned int div) {
  */
  pm_gc_setup(pm,
              EXAMPLE_GCLK_ID,
              1,  // Use Osc (=0) or PLL (=1), here PLL
              0,  // Sel Osc0/PLL0 or Osc1/PLL1
              0,  // disable divisor
              0); // no divisor

  /* Enable Generic clock */
  pm_gc_enable(pm, EXAMPLE_GCLK_ID);

  /* Set the GCLOCK function to the GPIO pin */
  gpio_enable_module_pin(EXAMPLE_GCLK_PIN, EXAMPLE_GCLK_FUNCTION);
  

  /* Divide PBA clock by 2 from main clock (PBA clock = 48MHz/2 = 24MHz).
     Pheripheral Bus A clock divisor enable = 1
     Pheripheral Bus A select = 0
     Pheripheral Bus B clock divisor enable = 0
     Pheripheral Bus B select = 0
     High Speed Bus clock divisor enable = 0
     High Speed Bus select = 0
  */
  pm_cksel(pm, 1, 0, 0, 0, 0, 0);

  // Set one wait-state (WS) for flash controller. 0 WS access is up to 30MHz for HSB/CPU clock.
  // As we want to have 48MHz on HSB/CPU clock, we need to set 1 WS on flash controller.
  flashc_set_wait_state(1);

  pm_switch_to_clock(pm, AVR32_PM_MCSEL_PLL0); /* Switch main clock to 48MHz */
}


/* \brief Software Delay
 *
 */
static void software_delay(void)
{
  int i;
  for (i=0; i<1000000; i++);
}


/* \brief This is an example of how to configure and start a PLL from Osc0
 * then configure the GCLK output to GLCK_0_1, and switch the main clock to
 * PLL0 output.
 *
 */
int main(void)
{
  volatile avr32_pm_t* pm = &AVR32_PM;

  /* start PLL0 and switch main clock to PLL0 output */
  /* Also set-up a generic clock from PLL0 and output it to a gpio pin. */
  local_start_pll0(pm);

  /* Now toggle LED0 using a GPIO */
  while(1)
  {
    gpio_tgl_gpio_pin(LED0_GPIO);
    software_delay();
  }
}
