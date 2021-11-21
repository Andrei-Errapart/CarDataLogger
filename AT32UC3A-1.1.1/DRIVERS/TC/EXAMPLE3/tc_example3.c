/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 * \brief Timer/Counter example 3.
 *
 * This example will start a timer/counter and generate a "tick" interrupt each milli-second.
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
 * This example will start a timer/counter and generate a "tick" interrupt each milli-second.
 *
 * The selected timer input clock is the internal clock labelled TC2
 * referred to as TIMER_CLOCK2 in the datasheet. TIMER_CLOCK2 is connected to PBA/4 (cf datasheet)
 *
 * The 16-bit timer/counter channel will cycle from 0x0000 to RC. RC is initialized
 * to (PBA/4)/ 1000, so that an interrupt will be triggered every 1 ms. Upon interrupt,
 * the GPIO pin 0 is toggled thus producing a square signal of frequency 500Hz.
 * Check the GPIO pin 0 on an oscilloscope and see the square signal at frequency 500 Hz.
 *
 * \section files Main Files
 * - tc.c: TC driver;
 * - tc.h: TC driver header file;
 * - tc_example3.c: TC example 3.
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
#if __GNUC__
#  include "intc.h"
#endif
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "tc.h"
#include "usart.h"


#define FPBA    FOSC0
#define TC_CHANNEL    0

// To specify we have to print a new time
volatile static int print_sec = 1;

volatile U32 tc_tick=0;

#if __GNUC__
__attribute__((__interrupt__)) void tc_irq( void )
#elif __ICCAVR32__
/* TC Interrupt  */
#pragma handler = AVR32_TC_IRQ_GROUP, 1
__interrupt void tc_irq( void )
#endif

{
  // Increment the ms seconds counter
  tc_tick++;

  // clear the interrupt flag
  AVR32_TC.channel[TC_CHANNEL].sr;

  // specify that an interrupt has been raised
  print_sec = 1;
  // Toggle GPIO pin 0 (this pin is used as a regular GPIO pin).
  gpio_tgl_gpio_pin(0); // Toggle GPIO pin 0 (this pin is used as a regular GPIO pin).
}

/*!
 * \brief print_i function : convert the given number to an ASCII decimal representation.
 */
char *print_i(char *str, int n)
{
    int i = 10;

    str[i] = '\0';
    do
    {
      str[--i] = '0' + n%10;
      n /= 10;
    }while(n);

    return &str[i];
}

/*! \brief Main function. Execution starts here.
 */
int main(void)
{
  char temp[20];
  char *ptemp;

  volatile avr32_tc_t *tc = &AVR32_TC;

  // Options for waveform genration.
  static const tc_waveform_opt_t WAVEFORM_OPT =
  {
    .channel  = TC_CHANNEL,                        // Channel selection.

    .bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
    .beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
    .bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
    .bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

    .aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
    .aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
    .acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
    .acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

    .wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
    .enetrg   = FALSE,                             // External event trigger enable.
    .eevt     = 0,                                 // External event selection.
    .eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
    .cpcdis   = FALSE,                             // Counter disable when RC compare.
    .cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

    .burst    = FALSE,                             // Burst signal selection.
    .clki     = FALSE,                             // Clock inversion.
    .tcclks   = TC_CLOCK_SOURCE_TC2                // Internal source clock 2 - connected to PBA/4
  };

  static const tc_interrupt_t TC_INTERRUPT =
  {
    .etrgs = 0,
    .ldrbs = 0,
    .ldras = 0,
    .cpcs  = 1,
    .cpbs  = 0,
    .cpas  = 0,
    .lovrs = 0,
    .covfs = 0
  };

  // Switch main clock to external oscillator 0 (crystal) : 12MHz
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  static const gpio_map_t USART_GPIO_MAP =
  {
    {AVR32_USART1_RXD_0_PIN, AVR32_USART1_RXD_0_FUNCTION},
    {AVR32_USART1_TXD_0_PIN, AVR32_USART1_TXD_0_FUNCTION}
  };

  // USART options.
  static const usart_options_t USART_OPTIONS =
  {
    .baudrate     = 57600,
    .charlength   = 8,
    .paritytype   = USART_NO_PARITY,
    .stopbits     = USART_1_STOPBIT,
    .channelmode  = 0
  };

  // Assign GPIO pins to USART1.
  gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  // Initialize USART in RS232 mode at 12MHz.
  usart_init_rs232(&AVR32_USART1, &USART_OPTIONS, FPBA);

  // Clear a VT100 terminal screen
  usart_write_line(&AVR32_USART1, "\x1B[2J");
  // Welcome sentence
  usart_write_line(&AVR32_USART1, "ATMEL\r\n");
  usart_write_line(&AVR32_USART1, "AVR32 UC3 - TC example\r\n");

  Disable_global_interrupt();

  // The INTC driver has to be used only for GNU GCC for AVR32.
#if __GNUC__
  // Initialize interrupt vectors.
  INTC_init_interrupts();

  // Register the RTC interrupt handler to the interrupt controller.
  INTC_register_interrupt(&tc_irq, AVR32_TC_IRQ0, INT0);
#endif

  Enable_global_interrupt();

  // Initialize the timer/counter.
  tc_init_waveform(tc, &WAVEFORM_OPT);         // Initialize the timer/counter waveform.

  // Set the compare triggers.
  // Remember TC counter is 16-bits, so counting second is not possible.
  // We configure it to count ms.
  // We want: (1/(FPBA/4)) * RC = 1000 Hz => RC = (FPBA/4) / 1000 = 3000 to get an interrupt every 1ms
  tc_write_rc(tc, TC_CHANNEL, (FPBA/4)/1000);  // Set RC value.

  tc_configure_interrupts(tc, TC_CHANNEL, &TC_INTERRUPT);

  // Start the timer/counter.
  tc_start(tc, TC_CHANNEL);                    // And start the timer/counter.

  while(1)
  {
    // Update the display on USART every second.
    if ((print_sec) && (!(tc_tick%1000)))
    {
      // Set cursor to the position (1; 5)
      usart_write_line(&AVR32_USART1, "\x1B[5;1H");
      ptemp = print_i(temp, tc_tick);
      usart_write_line(&AVR32_USART1, "Timer: ");
      usart_write_line(&AVR32_USART1, ptemp);
      usart_write_line(&AVR32_USART1, " ms");
      print_sec = 0;
    }
  }
}
