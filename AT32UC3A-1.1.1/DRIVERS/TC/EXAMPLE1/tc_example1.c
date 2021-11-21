/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 * \brief Timer/Counter example 1.
 *
 * This example will start a timer/counter and generate a PWM on the output.
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a TC module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

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
 * typedefs for the TC driver. <BR>It also gives an example of the usage of the
 * TC module.<BR>
 *
 * This example will start a timer/counter and generate a PWM on the output.
 *
 * Channel 0 of timer/counter module 0 is used with the output pin TIOA0.
 * You will find the port and pin number in the datasheet of your device.
 * The selected timer input clock is the internal clock labelled TC2
 * referred to as TIMER_CLOCK2 in the datasheet. Use an oscilloscope to probe the
 * TC channel's output pin.
 * \note
 * - On the AT32UC3A0512, the output pin TIOA0 is mapped on PB23. \n
 * - On the AT32UC3B0256, the output pin TIOA0 is mapped on PB00.
 *
 * The 16-bit timer/counter value register (CV) will cycle from 0x0000 to 0xFFFF.
 * The chosen channel with a low output pin level upon starting point (i.e. when
 * CV == 0) will toggle the output pin level when it reaches the values 0x0600
 * and 0x2000. This will hence produce a PWM output signal with a duty cycle of
 * (0x2000 - 0x600) / 0x10000 i.e. 10.16%
 * \note According to datasheet, TIMER_CLOCK2 is equal to (clk_pba/4). Since we
 * switch the main CPU clock to OSC0 (=12MHz), we have TIMER_CLOCK2 == 3MHz.\n
 * According to the settings performed by the application, the output waveform
 * should thus have the following properties:
 * - The period of the waveform will thus be 3000000/65535 == 45.7Hz,
 * - A full duty cycle(just toggling the channel's output pin when CV reaches
 * 0xFFFF and passes to 0x0000) thus lasts 21.845ms(65535/3000000). A duty cycle
 * of 10.16% means that when probing the channel's output pin you should see a
 * duty cycle of ~2.2ms(10.16% of 21.845ms).
 *
 * \section files Main Files
 * - tc.c: TC driver;
 * - tc.h: TC driver header file;
 * - tc_example1.c: TC example 1.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a TC module can be used. This example has been tested
 * with the following boards:<BR>
 * - EVK1100 evaluation kit with an AT32UC3A0512 MCU.
 * - EVK1101 evaluation kit with an AT32UC3B0256 MCU.
 *
 * \section setupinfo Setup Information
 * CPU speed: <i> 12 MHz. </i>
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */



#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "tc.h"


/*! \name TC channel choice
 */
//! @{
#if BOARD == EVK1100
#  define EXAMPLE_TC_CHANNEL_ID         0
#  define EXAMPLE_TC_CHANNEL_PIN        AVR32_TC_A0_0_PIN
#  define EXAMPLE_TC_CHANNEL_FUNCTION   AVR32_TC_A0_0_FUNCTION
// Note that TC_A0_0 pin is pin 55(PB23) on AT32UC3A0512 QFP144.
#elif BOARD == EVK1101
#  define EXAMPLE_TC_CHANNEL_ID         0
#  define EXAMPLE_TC_CHANNEL_PIN        AVR32_TC_A0_0_0_PIN
#  define EXAMPLE_TC_CHANNEL_FUNCTION   AVR32_TC_A0_0_0_FUNCTION
// Note that TC_A0_0_0 pin is pin 32(PB00) on AT32UC3B0256 QFP64.
#endif
//! @}


/*! \brief Main function. Execution starts here.
 */
int main(void)
{
  // The timer/counter instance and channel number are used in several functions.
  // It's defined as local variable for ease-of-use causes and readability.
  volatile avr32_tc_t *tc = &AVR32_TC;

  // Options for waveform genration.
  tc_waveform_opt_t waveform_opt =
  {
    .channel  = EXAMPLE_TC_CHANNEL_ID,        // Channel selection.

    .bswtrg   = TC_EVT_EFFECT_NOOP,           // Software trigger effect on TIOB.
    .beevt    = TC_EVT_EFFECT_NOOP,           // External event effect on TIOB.
    .bcpc     = TC_EVT_EFFECT_NOOP,           // RC compare effect on TIOB.
    .bcpb     = TC_EVT_EFFECT_NOOP,           // RB compare effect on TIOB.

    .aswtrg   = TC_EVT_EFFECT_NOOP,           // Software trigger effect on TIOA.
    .aeevt    = TC_EVT_EFFECT_NOOP,           // External event effect on TIOA.
    .acpc     = TC_EVT_EFFECT_TOGGLE,         // RC compare effect on TIOA: toggle.
    .acpa     = TC_EVT_EFFECT_TOGGLE,         // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

    .wavsel   = TC_WAVEFORM_SEL_UP_MODE,      // Waveform selection: Up mode without automatic trigger on RC compare.
    .enetrg   = FALSE,                        // External event trigger enable.
    .eevt     = TC_EXT_EVENT_SEL_TIOB_INPUT,  // External event selection.
    .eevtedg  = TC_SEL_NO_EDGE,               // External event edge selection.
    .cpcdis   = FALSE,                        // Counter disable when RC compare.
    .cpcstop  = FALSE,                        // Counter clock stopped with RC compare.

    .burst    = TC_BURST_NOT_GATED,           // Burst signal selection.
    .clki     = TC_CLOCK_RISING_EDGE,         // Clock inversion.
    .tcclks   = TC_CLOCK_SOURCE_TC2           // Internal source clock 2.
  };

  // Switch main clock to external oscillator 0 (crystal).
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Assign I/O to timer/counter channel pin & function.
  gpio_enable_module_pin(EXAMPLE_TC_CHANNEL_PIN, EXAMPLE_TC_CHANNEL_FUNCTION);

  // Initialize the timer/counter.
  tc_init_waveform(tc, &waveform_opt);  // Initialize the timer/counter waveform.

  // Set the compare triggers.
  tc_write_ra(tc, EXAMPLE_TC_CHANNEL_ID, 0x0600);     // Set RA value.
  tc_write_rc(tc, EXAMPLE_TC_CHANNEL_ID, 0x2000);     // Set RC value.

  // Start the timer/counter.
  tc_start(tc, EXAMPLE_TC_CHANNEL_ID);

  while (TRUE);
}
