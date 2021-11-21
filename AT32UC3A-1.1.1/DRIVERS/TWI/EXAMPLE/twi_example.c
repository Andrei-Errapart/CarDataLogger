/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief TWI example driver for AVR32 UC3.
 *
 * This file provides an example for the TWI on AVR32 UC3 devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a TWI module can be used.
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
 * typedefs for the TWI driver. <BR>It also gives an example of the usage of the
 * TWI module.\n
 * <b>Example's operating mode:</b>
 * - Probe the TWI component
 * - Write 8 data bytes to a TWI component at chip address 0x50 and internal address 0x0
 * - Read 8 data bytes from a TWI component at chip address 0x50 and internal address 0x0
 * - Check if data sent and data received are consistent.
 * - Display the test result on USART_1.
 *
 * \section files Main Files
 * - twi.c: TWI driver;
 * - twi.h: TWI driver header file;
 * - twi_example.c: TWI code example.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32UC devices with a TWI module can be used. This example has been tested
 * with the following setup:<BR>
 * - EVK1100 evaluation kit
 * - DP8582 EEPROM at address 0x50
 *
 * \section setupinfo Setup Information
 * <BR>CPU speed: <i> 12 MHz </i>
 * - Connect USART_1 to your serial port via a standard RS-232 D-SUB9 cable
 * - Set the following settings in your terminal of choice: 57600 8N1
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */


#include <avr32/io.h>
#include "board.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "count.h"
#include "intc.h"
#include "twi.h"


#define TEST_PATTERN          0x41
#define DEVICE_ADDRESS        0x50
#define PAGE_START_ADDRESS    0
#define NB_BYTES              8 // if EEPROM used, 8 bits max per write cycle


//! Flag set when time out starts and cleared when timeout occurs.
volatile unsigned char TimeOut = 0;

/*! \brief Interrupt handler for compare IT.
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void compare_irq_handler(void)
{
  TimeOut = 1;
  // Disable the compare and exception generation feature: set COMPARE to 0.
  Set_sys_compare(0);
}

/*! \brief Software delay.
 */
void delay_ms(unsigned short time_ms)
{
unsigned long u32CountVal,u32CompareVal;

  TimeOut = 0;

  u32CountVal = Get_sys_count();

  u32CompareVal = u32CountVal + ((unsigned long)time_ms * (FOSC0 / 1000)); // WARNING: MUST FIT IN 32bits.

  Set_sys_compare(u32CompareVal); // GO

  //  The previous COMPARE write succeeded.
  // Loop until the COUNT&COMPARE match triggers.
  while (!TimeOut);
}


/*! \brief Main function.
 */
int main(void)
{
  static const gpio_map_t TWI_GPIO_MAP =
  {
    {AVR32_TWI_SDA_0_0_PIN, AVR32_TWI_SDA_0_0_FUNCTION},
    {AVR32_TWI_SCL_0_0_PIN, AVR32_TWI_SCL_0_0_FUNCTION}
  };

  twi_options_t opt;
  twi_package_t packet, packet_received;
  int status, i;
  char data_sent[NB_BYTES];
  char data_received[NB_BYTES] = {0};

  // Init data to send
  for (i = 0 ; i <= NB_BYTES ; i++)
  {
    data_sent[i] = TEST_PATTERN + i;
  }

  // Switch to oscillator 0
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Disable all interrupts.
  Disable_global_interrupt();

  // Init the interrupt controller
  INTC_init_interrupts();

  // Register the compare interrupt handler to the interrupt controller.
  INTC_register_interrupt(&compare_irq_handler, AVR32_CORE_COMPARE_IRQ, INT0);

  // Enable all interrupts.
  Enable_global_interrupt();

  // Init debug serial line
  init_dbg_rs232(FOSC0);

  // Display a header to user
  print_dbg("\x1B[2J\x1B[H\r\nTWI Example\r\n");

  // TWI gpio pins configuration
  gpio_enable_module(TWI_GPIO_MAP, sizeof(TWI_GPIO_MAP) / sizeof(TWI_GPIO_MAP[0]));

  // options settings
  opt.pba_hz = FOSC0;
  opt.speed = 50000;
  opt.chip = DEVICE_ADDRESS;

  // initialize TWI driver with options
  status = twi_init(&AVR32_TWI, &opt);

  // check init result
  if (status == TWI_SUCCESS)
  {
    // display test result to user
    print_dbg("Probe test:\tPASS\r\n");
  }
  else
  {
    // display test result to user
    print_dbg("Probe test:\tFAIL\r\n");
  }

  // TWI chip address to communicate with
  packet.chip = DEVICE_ADDRESS;
  // TWI address/commands to issue to the other chip (node)
  packet.addr = PAGE_START_ADDRESS;
  // Length of the TWI data address segment (1-3 bytes)
  packet.addr_length = 1;
  // Where to find the data to be written
  packet.buffer = data_sent;
  // How many bytes do we want to write
  packet.length = NB_BYTES;

  // perform a write access
  status = twi_master_write(&AVR32_TWI, &packet);

  // check write result
  if (status == TWI_SUCCESS)
  {
    // display test result to user
    print_dbg("Write test:\tPASS\r\n");
  }
  else
  {
    // display test result to user
    print_dbg("Write test:\tFAIL\r\n");
  }

  // Accessing an EEPROM needs a guard time for bytes writing
  for (i = 0 ; i <= NB_BYTES ; i++)
  {
    delay_ms(7);
  }

  // TWI chip address to communicate with
  packet_received.chip = DEVICE_ADDRESS ;
  // Length of the TWI data address segment (1-3 bytes)
  packet_received.addr_length = 1;
  // How many bytes do we want to write
  packet_received.length = NB_BYTES;
  // TWI address/commands to issue to the other chip (node)
  packet_received.addr = PAGE_START_ADDRESS;
  // Where to find the data to be written
  packet_received.buffer = data_received;

  // perform a read access
  status = twi_master_read(&AVR32_TWI, &packet_received);

  // check read result
  if (status == TWI_SUCCESS)
  {
    // display test result to user
    print_dbg("Read Test:\tPASS\r\n");
  }
  else
  {
    // display test result to user
    print_dbg("Read test:\tFAIL\r\n");
  }

  // check received data against sent data
  for (i = 0 ; i < NB_BYTES ; i++)
  {
    if (packet_received.buffer[i] != packet.buffer[i])
    {
      // a char isn't consistent
      print_dbg("Check Read:\tFAIL\r\n");
      // Error
      while(1);
    }
  }

  // everything was OK
  print_dbg("Check Read:\tPASS\r\n");
  while(1)
  {
    // LED OK blinks
    gpio_tgl_gpio_pin(LED0_GPIO);
    delay_ms(500);
  }
}
