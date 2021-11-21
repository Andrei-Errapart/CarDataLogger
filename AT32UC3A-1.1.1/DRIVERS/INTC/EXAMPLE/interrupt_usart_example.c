/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief Example of an interrupt-driven USART communication.
 *
 * This file contains an example using the USART module with interrupts.
 *
 * It will display a string on the screen. Refer to the usart_options structure
 * in main for configuration options for your terminal.
 *
 * Please make sure that the correct jumper(s) is/are set on your development
 * board. Refer to your hardware reference guide if necessary.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an INTC module can be used.
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
 * This is the documentation for the data structures, functions, variables,
 * defines, enums, and typedefs for the interrupt controller driver. It also comes
 * bundled with a basic application as an example of usage.\n
 * This example demonstrates how to use the INTC driver for the USART RXRDY interrupt.
 * <b> Operating mode: </b>each character typed on the PC terminal is echoed back.
 *
 * \note The IAR built-in event handling mechanism manages interrupt groups but not lines.
 * This example shows how to register an interrupt through the INTC driver, which manages lines.
 * If interrupt request lines do not have to be handled separately within groups,
 * then the IAR built-in event handling mechanism can be used in this way:
 * - remove the INTC driver files (intc.c and exception.s82) from the IAR project;
 * - remove calls to the INTC driver functions (\ref INTC_init_interrupts and \ref INTC_register_interrupt);
 * - add <tt>\#pragma handler = \e AVR32_xxxx_IRQ_GROUP, \e int_level</tt> above the \c __interrupt keyword;
 * - the IAR project options can be changed to catch unhandled events under the \e Runtime tab of
 *   the <i>General Options</i> item.
 *
 * \warning The IAR built-in event handling mechanism (<tt>\#pragma exception</tt> and
 *          <tt>\#pragma handler</tt>) MUST NOT be used along with the INTC driver
 *          within a project.
 *
 * \section files Main Files
 * - intc.c: interrupt controller driver;
 * - intc.h: interrupt controller driver header file;
 * - exception.S: exception and interrupt handlers assembly file for GCC;
 * - interrupt_usart_example.c: interrupt controller example application.
 *
 * \section compilinfo Compilation Information
 * This software is written for GNU GCC for AVR32 and for IAR Embedded Workbench
 * for Atmel AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Information
 * All AVR32 devices with an INTC and a USART module can be used.
 *
 * \section configinfo Configuration Information
 * This example has been tested with the following configuration:
 * - EVK1100 or EVK1101 evaluation kit;
 * - CPU clock: 12 MHz;
 * - USART1 (on EVK1100) or USART0 (on EVK1101) connected to a PC serial port via a standard RS232 DB9 cable;
 * - PC terminal settings:
 *   - 57600 bps,
 *   - 8 data bits,
 *   - no parity bit,
 *   - 1 stop bit,
 *   - no flow control.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */

#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "print_funcs.h"
#include "intc.h"
#include "pm.h"
#include "gpio.h"
#include "usart.h"


/*! \name USART Settings
 */
//! @{
#if BOARD == EVK1100
#  define EXAMPLE_USART               (&AVR32_USART0)
#  define EXAMPLE_USART_RX_PIN        AVR32_USART0_RXD_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION   AVR32_USART0_RXD_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN        AVR32_USART0_TXD_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION   AVR32_USART0_TXD_0_FUNCTION
#  define EXAMPLE_USART_IRQ           AVR32_USART0_IRQ
#elif BOARD == EVK1101
#  define EXAMPLE_USART               (&AVR32_USART1)
#  define EXAMPLE_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#  define EXAMPLE_USART_IRQ           AVR32_USART1_IRQ
#endif
//! @}


/*! \brief The USART interrupt handler.
 *
 * \note The `__attribute__((__interrupt__))' (under GNU GCC for AVR32) and
 *       `__interrupt' (under IAR Embedded Workbench for Atmel AVR32) C function
 *       attributes are used to manage the `rete' instruction.
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void usart_int_handler(void)
{
  int c;

  // In the code line below, the interrupt priority level does not need to be
  // explicitly masked as it is already because we are within the interrupt
  // handler.
  // The USART Rx interrupt flag is cleared by side effect when reading the
  // received character.
  // Waiting until the interrupt has actually been cleared is here useless as
  // the call to usart_write_char will take enough time for this before the
  // interrupt handler is leaved and the interrupt priority level is unmasked by
  // the CPU.
  usart_read_char(EXAMPLE_USART, &c);

  // Print the received character to USART.
  // It is a simple echo, so there will be no translation of '\r' to "\r\n". The
  // connected terminal has to be configured accordingly to send '\n' after
  // '\r'.
  usart_write_char(EXAMPLE_USART, c);
}


/*! \brief The main function.
 *
 * It sets up the USART module on EXAMPLE_USART. The terminal settings are 57600
 * 8N1.
 * Then it sets up the interrupt handler and waits for a USART interrupt to
 * trigger.
 */
int main(void)
{
  static const gpio_map_t USART_GPIO_MAP =
  {
    {EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
    {EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
  };

  // USART options.
  static const usart_options_t USART_OPTIONS =
  {
    .baudrate     = 57600,
    .charlength   = 8,
    .paritytype   = USART_NO_PARITY,
    .stopbits     = USART_1_STOPBIT,
    .channelmode  = USART_NORMAL_CHMODE
  };

  // Switch main clock to external oscillator 0 (crystal).
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Assign GPIO to USART.
  gpio_enable_module(USART_GPIO_MAP,
                     sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  // Initialize USART in RS232 mode.
  usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, FOSC0);
  print(EXAMPLE_USART, ".: Using interrupts with the USART :.\n\n");

  // Disable all interrupts.
  Disable_global_interrupt();

  // Initialize interrupt vectors.
  INTC_init_interrupts();

  // Register the USART interrupt handler to the interrupt controller.
  // usart_int_handler is the interrupt handler to register.
  // EXAMPLE_USART_IRQ is the IRQ of the interrupt handler to register.
  // INT0 is the interrupt priority level to assign to the group of this IRQ.
  // void INTC_register_interrupt(__int_handler handler, unsigned int irq, unsigned int int_lev);
  INTC_register_interrupt(&usart_int_handler, EXAMPLE_USART_IRQ, INT0);

  // Enable USART Rx interrupt.
  EXAMPLE_USART->ier = AVR32_USART_IER_RXRDY_MASK;
  print(EXAMPLE_USART, "Type a character to use the interrupt handler.\n"
                       "It will show up on your screen.\n\n");

  // Enable all interrupts.
  Enable_global_interrupt();

  while (TRUE);
}
