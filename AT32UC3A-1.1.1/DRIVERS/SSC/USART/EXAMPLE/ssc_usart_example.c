/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief Example of an interrupt-driven SSC communication.
 *
 * This file contains an example using the SSC module with interrupts.
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
 * This is the documentation for the data structures, functions, variables, defines, enums, and
 * typedefs for the SSC USART emulation driver. <BR>It also comes with an example
 * of the usage of this driver: <BR>
 * - Open an hyperterminal connected to USART 0 (the terminal settings are 57600 8N1).
 * - Type a character : it is echoed back by the application.<BR>
 *
 * \image html ssc_usart_example.jpg
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
 * - ssc_usart.c: SSC driver;
 * - ssc_usart.h: SSC driver header file;
 * - ssc_usart_example.c: SSC example application.
 *
 * \section compilinfo Compilation Information
 * This software is written for GNU GCC for AVR32 and for IAR Embedded Workbench
 * for Atmel AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Information
 * All AVR32 devices with a USART module can be used.
 *
 * \section configinfo Configuration Information
 * This example has been tested with the following configuration:
 * - EVK1100 or EVK1101 evaluation kit;
 * - CPU clock: 12 MHz;
 * - USART0 (on EVK1100) or USART1 (on EVK1101) connected to a PC serial port via a standard RS232 DB9 cable;
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
#include "board.h"
#include "compiler.h"
#include "ssc_usart.h"
#include "usart.h"
#include "gpio.h"
#include "pm.h"
#include "intc.h"
#include "print_funcs.h"


/*! \name USART Settings
 */
//! @{
#if BOARD == EVK1100
#  define EXAMPLE_USART             (&AVR32_USART0)
#  define EXAMPLE_USART_RX_PIN      AVR32_USART0_RXD_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION AVR32_USART0_RXD_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN      AVR32_USART0_TXD_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION AVR32_USART0_TXD_0_FUNCTION
#  define EXAMPLE_USART_IRQ         AVR32_USART0_IRQ
#elif BOARD == EVK1101
#  define EXAMPLE_USART             (&AVR32_USART1)
#  define EXAMPLE_USART_RX_PIN      AVR32_USART1_RXD_0_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION AVR32_USART1_RXD_0_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN      AVR32_USART1_TXD_0_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION AVR32_USART1_TXD_0_0_FUNCTION
#  define EXAMPLE_USART_IRQ         AVR32_USART1_IRQ
#endif
//! @}

/* SSC peripheral registers address. */
volatile avr32_ssc_t *ssc = &AVR32_SSC;

/*! \brief The SSC interrupt handler.
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
static void ssc_int_handler(void)
{
  int c;

  // In the code line below, the interrupt priority level does not need to be
  // explicitly masked as it is already because we are within the interrupt
  // handler.
  // The SSC Rx interrupt flag is cleared by side effect when reading the
  // received character.
  // Waiting until the interrupt has actually been cleared is here useless as
  // the call to print_char will take a very long time before the interrupt
  // handler is left and the interrupt priority level is unmasked by the CPU.

  // Get the received character on the SSC.
  c = ssc_uart_getchar(ssc);
  // Print a msg to USART.
  print(EXAMPLE_USART,"\n -- SSC Rx Interrupt -- ");
  print(EXAMPLE_USART,"\n Data received : ");
  // Print the received character to USART.
  print_char(EXAMPLE_USART, (c == '\r') ? '\n' : c);
}

/*! \brief The USART interrupt handler.
 *
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
  // the call to print_char will take a very long time before the interrupt
  // handler is left and the interrupt priority level is unmasked by the CPU.

  // Get the received character on the USART.
  usart_read_char(EXAMPLE_USART, &c);
  // Print a msg to USART.
  print(EXAMPLE_USART,"\n -- USART Rx Interrupt -- ");
  print(EXAMPLE_USART,"\n Data transmitted : ");
  // Print the received character to USART.
  print_char(EXAMPLE_USART, (c == '\r') ? '\n' : c);

  // Transmit data received through to SSC
  ssc_uart_putchar(ssc,c);
}


/*! \brief The main function.
 *
 * It sets up the SSC module on EXAMPLE_SSC. The terminal settings are 57600
 * 8N1.
 * Then it sets up the interrupt handler and waits for a SSC interrupt to
 * trigger.
 */
int main(void)
{
  // USART GPIO mapping.
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

  // SSC GPIO mapping.
  static const gpio_map_t SSC_GPIO_MAP =
  {
    {AVR32_SSC_RX_DATA_0_PIN, AVR32_SSC_RX_DATA_0_FUNCTION},
    {AVR32_SSC_TX_DATA_0_PIN, AVR32_SSC_TX_DATA_0_FUNCTION}
  };

  // Swith to external Oscillator 0.
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
  
  // Assign GPIO pins to USART.
  gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  // Initialize USART in RS232 mode.
  usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, FOSC0);

  // Assign GPIO pins to SSC.
  gpio_enable_module(SSC_GPIO_MAP, sizeof(SSC_GPIO_MAP) / sizeof(SSC_GPIO_MAP[0]));
  
  // Initializes SSC in USART mode. Note: we enable the loopback mode for the sake
  // of this example (the SSC receiver is programmed to receive transmissions from
  // the SSC transmitter).
  ssc_uart_init( ssc, 57600, SSC_UART_STOPBITS_ONE, SSC_UART_PARITY_MARK , 8,
                 SSC_UART_LOOPBACK_MODE_ENABLE, FOSC0);

  // Disable all interrupts.
  Disable_global_interrupt();

  // Initialize interrupt vectors.
  INTC_init_interrupts();

  // Register the SSC interrupt handler to the interrupt controller.
  INTC_register_interrupt(&ssc_int_handler, AVR32_SSC_IRQ, INT0);
  // Register the USART interrupt handler to the interrupt controller.
  INTC_register_interrupt(&usart_int_handler, EXAMPLE_USART_IRQ, INT1);

  // Enable SSC Rx interrupt.
  ssc->ier = (1<<AVR32_SSC_IER_RXRDY_OFFSET);
  // Enable USART Rx interrupt.
  EXAMPLE_USART->ier = AVR32_USART_IER_RXRDY_MASK;
  
  // Print a message to USART.
  print(EXAMPLE_USART, "\n\nType a character to use the interrupt handler.\n"
                       "It will be sent through SSC.\n"
                       "An interrupt occurs at the end of reception.\n\n" );

  // Enable all interrupts.
  Enable_global_interrupt();

  while(TRUE);
}

