/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 * \brief Timer/Counter example 2.
 *
 * This example involves 2 timer/counter channels, one configured in capture
 * mode(input) and the other configured in Waveform mode(output) to generate a
 * PWM on the output.
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
 * This example involves 2 timer/counter channels, one configured in capture
 * mode(input) and the other configured in Waveform mode(output) to generate a
 * PWM on the output.
 *
 * Channel 1 of timer/counter module 0 is used with the input pin TIOA1. You
 * will find the port and pin number in the datasheet of your device. The
 * selected timer input clock is the internal clock labelled TC5 referred to
 * as TIMER_CLOCK5 in the datasheet.
 *
 * Channel 0 of timer/counter module 0 is used with the output pin TIOA0.
 * You will find the port and pin number in the datasheet of your device.
 * The selected timer input clock is the internal clock labelled TC4
 * referred to as TIMER_CLOCK4 in the datasheet.
 *
 * The 16-bit input timer/counter channel will cycle from 0x0000 to 0xFFFF
 * or until a falling edge is detected on the input pin, in which case the
 * counter is captured in RA before being reset then started with the same
 * rules. Consequently, RA is a measure of the input period modulo 16 bits.
 *
 * The 16-bit output timer/counter channel will cycle from 0x0000 to 0xFFFF,
 * starting at 0x0000 with a high output pin level and lowering the output
 * pin level when it reaches the value of RA extracted from the input timer/
 * counter while raising the output pin level when it reaches the value of
 * RC left at 0. This will hence produce a PWM output signal with a duty
 * cycle depending on the period of the input signal.
 *
 * \section files Main Files
 * - tc.c: TC driver;
 * - tc.h: TC driver header file;
 * - tc_example2.c: TC example 2.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a TC module can be used. This example has been tested
 * with the following board:<BR>
 * - EVK1100 evaluation kit
 *
 * \section setupinfo Setup Information
 * <BR>CPU speed: <i> 12 MHz. </i>
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


/*! \brief Initializes the timer/counter capture.
 */
static void init_tc_input(volatile avr32_tc_t *tc, unsigned int channel)
{
  // Options for capture mode.
  tc_capture_opt_t capture_opt =
  {
    .channel  = channel,                      // Channel selection.

    .ldrb     = TC_SEL_NO_EDGE,               // RB loading selection.
    .ldra     = TC_SEL_FALLING_EDGE,          // RA loading selection.

    .cpctrg   = TC_NO_TRIGGER_COMPARE_RC,     // RC compare trigger enable.
    .abetrg   = TC_EXT_TRIG_SEL_TIOA,         // TIOA or TIOB external trigger selection.
    .etrgedg  = TC_SEL_FALLING_EDGE,          // External trigger edge selection.

    .ldbdis   = FALSE,                        // Counter clock disable with RB loading.
    .ldbstop  = FALSE,                        // Counter clock stopped with RB loading.

    .burst    = TC_BURST_NOT_GATED,           // Burst signal selection.
    .clki     = TC_CLOCK_RISING_EDGE,         // Clock inversion.
    .tcclks   = TC_CLOCK_SOURCE_TC5           // Internal source clock 5  - connected to PBA/32
  };

  // Initialize the timer/counter capture.
  tc_init_capture(tc, &capture_opt);
}


/*! \brief Initializes the timer/counter waveform.
 */
static void init_tc_output(volatile avr32_tc_t *tc, unsigned int channel)
{
  // Options for waveform generation.
  tc_waveform_opt_t waveform_opt =
  {
    .channel  = channel,                      // Channel selection.

    .bswtrg   = TC_EVT_EFFECT_NOOP,           // Software trigger effect on TIOB.
    .beevt    = TC_EVT_EFFECT_NOOP,           // External event effect on TIOB.
    .bcpc     = TC_EVT_EFFECT_NOOP,           // RC compare effect on TIOB.
    .bcpb     = TC_EVT_EFFECT_NOOP,           // RB compare effect on TIOB.

    .aswtrg   = TC_EVT_EFFECT_NOOP,           // Software trigger effect on TIOA.
    .aeevt    = TC_EVT_EFFECT_NOOP,           // External event effect on TIOA.
    .acpc     = TC_EVT_EFFECT_SET,            // RC compare effect on TIOA.
    .acpa     = TC_EVT_EFFECT_CLEAR,          // RA compare effect on TIOA.

    .wavsel   = TC_WAVEFORM_SEL_UP_MODE,      // Waveform selection: Up mode without automatic trigger on RC compare.
    .enetrg   = FALSE,                        // External event trigger enable.
    .eevt     = TC_EXT_EVENT_SEL_TIOB_INPUT,  // External event selection.
    .eevtedg  = TC_SEL_NO_EDGE,               // External event edge selection.
    .cpcdis   = FALSE,                        // Counter disable when RC compare.
    .cpcstop  = FALSE,                        // Counter clock stopped with RC compare.

    .burst    = TC_BURST_NOT_GATED,           // Burst signal selection.
    .clki     = TC_CLOCK_RISING_EDGE,         // Clock inversion.
    .tcclks   = TC_CLOCK_SOURCE_TC4           // Internal source clock 4 -  - connected to PBA/16
  };

  // Initialize the timer/counter waveform.
  tc_init_waveform(tc, &waveform_opt);
}


/*! \brief Main function. Execution starts here.
 */
int main(void)
{
  static const gpio_map_t TC_GPIO_MAP =
  {
    // Assign I/O to timer/counter TIOA1 pin function (input):
    // optional as far as the port pin is not driven by the MCU.
    {AVR32_TC_A1_0_PIN, AVR32_TC_A1_0_FUNCTION},
    // On the AT32UC3A0512, the input pin TIOA1 is mapped on PB25.

    // Assign I/O to timer/counter TIOA0 pin function (output).
    {AVR32_TC_A0_0_PIN, AVR32_TC_A0_0_FUNCTION}
    // On the AT32UC3A0512, the output pin TIOA0 is mapped on PB23.
  };

  // The timer/counter instance and channel numbers are used in several functions.
  // It's defined as local variable for ease-of-use causes and readability.
  volatile avr32_tc_t *tc = &AVR32_TC;
  unsigned int input_channel = 1;
  unsigned int output_channel = 0;

  // Used to read the RA value for the input timer/counter instance.
  int ra = 0;

  // Switch main clock to external oscillator 0 (crystal).
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Assign I/Os to timer/counter.
  gpio_enable_module(TC_GPIO_MAP, sizeof(TC_GPIO_MAP) / sizeof(TC_GPIO_MAP[0]));

  // Initialize the timers/counters.
  init_tc_input(tc, input_channel);
  init_tc_output(tc, output_channel);

  // Set the compare trigger.
  tc_write_ra(tc, output_channel, 0x2000);

  // Start the timers/counters.
  tc_start(tc, input_channel);
  tc_start(tc, output_channel);

  while (TRUE)
  {
    ra = tc_read_ra(tc, input_channel);
    if (ra > 0)
    {
      // RA of the input channel has changed, so it has detected a falling edge.
      // Update the RA of the output channel.
      tc_write_ra(tc, output_channel, ra);
    }
  }
}
