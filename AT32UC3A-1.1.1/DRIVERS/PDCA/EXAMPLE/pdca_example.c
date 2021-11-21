/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief PDCA and interrupt control example.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with PDCA and USART modules.
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
 * This documents data structures, functions, variables, defines, enums, and
 * typedefs in the software for the PDCA driver.
 *
 * The Peripheral DMA controller (PDCA) transfers data between on-chip peripheral modules such
 * as USART, SPI, SSC and on- and off-chip memories. Using the PDCA avoids CPU intervention
 * for data transfers, improving the performance of the microcontroller. The PDCA can transfer
 * data from memory to a peripheral or from a peripheral to memory.
 *
 * The given example is a transfer of an ASCII animation stored in internal flash to the USART_0 output.
 * The ASCII animation is divived in two files (ascii_anim1.h and ascii_anim2.h).
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section files Main Files
 * - pdca.c: PDCA driver
 * - pdca.h: PDCA header file
 * - pdca_example.c: PDCA application example
 *
 * \section configinfo Configuration Information
 * This example has been tested with the following configuration:
 * - EVK1100 evaluation kit or EVK1101 evaluation kit;
 * - CPU clock: 12 MHz;
 * - USART1 connected to a PC serial port via a standard RS232 DB9 cable;
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
#if __GNUC__
#  include "intc.h"
#endif
#include "pdca.h"
#include "usart.h"
#include "gpio.h"
#include "board.h"
#include "pm.h"


#if BOARD == EVK1100
#  define EXAMPLE_USART               (&AVR32_USART1)
#  define EXAMPLE_USART_RX_PIN        AVR32_USART1_RXD_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION   AVR32_USART1_RXD_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN        AVR32_USART1_TXD_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION   AVR32_USART1_TXD_0_FUNCTION
#  define AVR32_PDCA_PID_USART_TX     AVR32_PDCA_PID_USART1_TX
#elif BOARD == EVK1101
#  define EXAMPLE_USART               (&AVR32_USART1)
#  define EXAMPLE_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#  define AVR32_PDCA_PID_USART_TX     AVR32_PDCA_PID_USART1_TX
#endif


//! The channel instance for the USART example, here PDCA channel 0 (highest priority).
#define PDCA_CHANNEL_USART_EXAMPLE 0

//! Counter for interrupt test.
volatile int bool_anim;

//! String for ASCII animation: this is the first part of the ASCII animation.
const char ascii_anim1[] =
#include "ascii_anim1.h"
;

//! String for ASCII animation: this is the second and final part of the ASCII animation.
const char ascii_anim2[] =
#include "ascii_anim2.h"
;


/*! \brief The PDCA interrupt handler.
 *
 * The handler reload the PDCA settings with the correct ASCII animation address and size using the reload register.
 * The interrupt will happen each time half of the animation is played.
 * Let's use interrupt level 0 in the example.
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
#pragma handler = AVR32_PDCA_IRQ_GROUP, 0
__interrupt
#endif
static void pdca_int_handler(void)
{

  if (bool_anim == 1)
  {
    // Set PDCA channel reload values with address where data to load are stored, and size of the data block to load.
    pdca_reload_channel(PDCA_CHANNEL_USART_EXAMPLE, (unsigned int)&ascii_anim2, sizeof( ascii_anim2 ));
    bool_anim = 2;
  }
  else if (bool_anim == 2)
  {
    pdca_reload_channel(PDCA_CHANNEL_USART_EXAMPLE, (unsigned int)&ascii_anim1, sizeof( ascii_anim1 ));
    bool_anim = 1;
  }
}


/*! \brief Init interrupt controller and register pdca_int_handler interrupt.
 */
void pdca_set_irq(void)
{
#if __GNUC__
  // Disable all interrupt/exception.
  Disable_global_interrupt();

  INTC_init_interrupts();

  // Register the compare interrupt handler to the interrupt controller
  // and enable the compare interrupt.
  // (__int_handler) &pdca_int_handler The handler function to register.
  // AVR32_PDCA_IRQ_0 The interrupt line to register to.
  // INT0  The priority level to set for this interrupt line.
  // INTC_register_interrupt(__int_handler handler, int line, int priority);
  INTC_register_interrupt( (__int_handler) &pdca_int_handler, AVR32_PDCA_IRQ_0, INT0);
#endif

  // Enable all interrupt/exception.
  Enable_global_interrupt();
}


/*! \brief USART test if transmit buffer is ready before launching a PDCA transfer.
 *
 * \param usart Pointer to a avr32_usart_t
 */
int usart_check_tx_status(volatile avr32_usart_t * usart)
{
  if (usart->csr & (1<<AVR32_USART_CSR_TXEMPTY_OFFSET)) return USART_SUCCESS;
  else return USART_TX_BUSY;
}


/*! \brief The given example is a transfer of an ASCII animation stored in internal flash to the USART_0 output (57600 bps/8 data bits/no parity bit/1 stop bit/no flow control)
 *
 * This example uses the USART, GPIO, INTC and PDCA modules.
 */
int main(void)
{
  volatile avr32_pdca_channel_t *pdca_channel; // get the correct channel pointer
  pdca_channel = pdca_get_handler(PDCA_CHANNEL_USART_EXAMPLE); // get the correct channel pointer

  // Switch to OSC0 as source of main clock.
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // As this program and the ASCII animation are loaded in internal Flash,
  // both the CPU instruction master and the PDCA master interface will access the flash at the same time.
  //
  // In order to avoid long slave handling during undefined length bursts (INCR), the Bus Matrix
  // provides specific logic in order to re-arbitrate before the end of the INCR transfer.
  //
  // HSB Bus Matrix: By default the HSB bus matrix mode is in Undefined length burst type (INCR).
  // Here we have to put in single access (the undefined length burst is treated as a succession of single
  // accesses, allowing re-arbitration at each beat of the INCR burst.
  // Refer to the HSB bus matrix section of the datasheet for more details.
  //
  // HSB Bus matrix register MCFG1 is associated with the CPU instruction master interface.
  AVR32_HMATRIX.mcfg[1] = 0x1;

  // Init counter for PDCA interrupt.
  bool_anim=1;

  static const gpio_map_t USART_GPIO_MAP =
  {
    {EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
    {EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
  };

  // USART options.
  static const usart_options_t USART_OPTIONS =
  {
    .baudrate	 = 57600,
    .charlength	 = 8,
    .paritytype	 = USART_NO_PARITY,
    .stopbits	 = USART_1_STOPBIT,
    .channelmode = USART_NORMAL_CHMODE,
  };

  // Assign GPIO pins to USART_0.
  gpio_enable_module(USART_GPIO_MAP,
                     sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  // Initialize the USART_0 in RS232 mode.
  usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, FOSC0);

  usart_write_line(EXAMPLE_USART, "PDCA Example.\n");  

  // PDCA channel options
  static const pdca_channel_options_t PDCA_OPTIONS =
  {
    .addr = (unsigned int) &ascii_anim1,   // memory address
    .pid = AVR32_PDCA_PID_USART_TX,       // select peripheral - data are transmit on USART TX line.
    .size = sizeof(ascii_anim1),           // transfer counter
    .r_addr = 0, // next memory address
    .r_size = 0,	   // next transfer counter
    .mode = PDCA_MODE_BYTE,                // select size of the transfer
  };

  // Init PDCA channel with the pdca_options.
  pdca_init_channel(PDCA_CHANNEL_USART_EXAMPLE, &PDCA_OPTIONS); // init PDCA channel with options.

  // Register PDCA IRQ interrupt.
  pdca_set_irq();
  
  // Enable pdca interrupt each time the reload counter reaches zero, i.e. each time
  // half of the ASCII animation (either anim1 or anim2) is transferred.
  pdca_enable_interrupt_reload_counter_zero(PDCA_CHANNEL_USART_EXAMPLE);

  // Enable now the transfer.
  pdca_enable(PDCA_CHANNEL_USART_EXAMPLE);

  while (1); // wait for the PDCA interrupt
}
