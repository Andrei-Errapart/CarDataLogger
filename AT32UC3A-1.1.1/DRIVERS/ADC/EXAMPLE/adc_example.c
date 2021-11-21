/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief ADC example driver for AVR32 UC3.
 *
 * This file provides an example for the ADC on AVR32 UC3 devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an ADC
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
 * typedefs for the ADC driver. <BR>It also gives an example of the usage of the
 * ADC module, eg: <BR>
 * - Use the temperature sensor and see the value change on USART_1,
 * - Use the light sensor and see the value change on USART_1,
 * - [on EVK1100 only] Use the adjustable resistor and see the value change on USART_1.<BR>
 *
 * \section files Main Files
 * - adc.c : ADC driver
 * - adc.h : ADC header file
 * - adc_example.c : ADC code example
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a ADC module can be used. This example has been tested
 * with the following setup:<BR>
 * - EVK1100 evaluation kit,
 * - EVK1101 evaluation kit.
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

#include "board.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "adc.h"


/*! \name ADC channels choice
 */
//! @{
#if BOARD == EVK1100
// Connection of the temperature sensor
#  define EXAMPLE_ADC_TEMPERATURE_CHANNEL     0
#  define EXAMPLE_ADC_TEMPERATURE_PIN         AVR32_ADC_AD_0_PIN
#  define EXAMPLE_ADC_TEMPERATURE_FUNCTION    AVR32_ADC_AD_0_FUNCTION
// Connection of the light sensor
#  define EXAMPLE_ADC_LIGHT_CHANNEL           2
#  define EXAMPLE_ADC_LIGHT_PIN               AVR32_ADC_AD_2_PIN
#  define EXAMPLE_ADC_LIGHT_FUNCTION          AVR32_ADC_AD_2_FUNCTION
// Connection of the potentiometer
#  define EXAMPLE_ADC_POTENTIOMETER_CHANNEL   1
#  define EXAMPLE_ADC_POTENTIOMETER_PIN       AVR32_ADC_AD_1_PIN
#  define EXAMPLE_ADC_POTENTIOMETER_FUNCTION  AVR32_ADC_AD_1_FUNCTION
// Note: Corresponding defines are defined in /BOARDS/EVK1100/evk1100.h.
// These are here for educational purposes only.
#elif BOARD == EVK1101
// Connection of the temperature sensor
#  define EXAMPLE_ADC_TEMPERATURE_CHANNEL     7
#  define EXAMPLE_ADC_TEMPERATURE_PIN         AVR32_ADC_AD_7_PIN
#  define EXAMPLE_ADC_TEMPERATURE_FUNCTION    AVR32_ADC_AD_7_FUNCTION
// Connection of the light sensor
#  define EXAMPLE_ADC_LIGHT_CHANNEL           6
#  define EXAMPLE_ADC_LIGHT_PIN               AVR32_ADC_AD_6_PIN
#  define EXAMPLE_ADC_LIGHT_FUNCTION          AVR32_ADC_AD_6_FUNCTION
// Note: Corresponding defines are defined in /BOARDS/EVK1101/evk1101.h.
// These are here for educational purposes only.
#endif
//! @}

/*!
 * \brief main function : do init and loop to display ADC values
 */
int main(int argc, char **argv)
{
  // GPIO pin/adc-function map.
  static const gpio_map_t ADC_GPIO_MAP =
  {
    {EXAMPLE_ADC_TEMPERATURE_PIN, EXAMPLE_ADC_TEMPERATURE_FUNCTION},
    {EXAMPLE_ADC_LIGHT_PIN, EXAMPLE_ADC_LIGHT_FUNCTION},
#if BOARD == EVK1100
    {EXAMPLE_ADC_POTENTIOMETER_PIN, EXAMPLE_ADC_POTENTIOMETER_FUNCTION}
#endif
  };

  volatile avr32_adc_t *adc = &AVR32_ADC; // ADC IP registers address

  signed short adc_value_temp = -1;
  signed short adc_value_light = -1;
#if BOARD == EVK1100
  signed short adc_value_pot = -1;
#endif

  // Assign the on-board sensors to their ADC channel.
  unsigned short adc_channel_temp = EXAMPLE_ADC_TEMPERATURE_CHANNEL;
  unsigned short adc_channel_light = EXAMPLE_ADC_LIGHT_CHANNEL;
#if BOARD == EVK1100
  unsigned short adc_channel_pot = EXAMPLE_ADC_POTENTIOMETER_CHANNEL;
#endif

  int i;


  // switch to oscillator 0
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // init debug serial line
  init_dbg_rs232(FOSC0);

  // Assign and enable GPIO pins to the ADC function.
  gpio_enable_module(ADC_GPIO_MAP, sizeof(ADC_GPIO_MAP) / sizeof(ADC_GPIO_MAP[0]));

  // configure ADC
  adc_configure(adc);

  // do a loop
  for (;;)
  {
    // slow down operations
    for ( i=0 ; i < 1000000 ; i++);

    // display a header to user
    print_dbg("\x1B[2J\x1B[H\r\nADC Example\r\n");

    // get value for first adc channel
    adc_enable(adc,adc_channel_temp);
    adc_start(adc);
    adc_value_temp = adc_get_value(adc, adc_channel_temp);
    adc_disable(adc,adc_channel_temp);
    // display value to user
    print_dbg("HEX Value for Channel temperature : 0x");
    print_dbg_hex(adc_value_temp);
    print_dbg("\r\n");

    // get value for second adc channel
    adc_enable(adc,adc_channel_light);
    adc_start(adc);
    adc_value_light = adc_get_value(adc, adc_channel_light);
    adc_disable(adc,adc_channel_light);
    // display value to user
    print_dbg("HEX Value for Channel light : 0x");
    print_dbg_hex(adc_value_light);
    print_dbg("\r\n");

#if BOARD == EVK1100
    // get value for third adc channel
    adc_enable(adc,adc_channel_pot);
    adc_start(adc);
    adc_value_pot = adc_get_value(adc, adc_channel_pot);
    adc_disable(adc,adc_channel_pot);
    // display value to user
    print_dbg("HEX Value for Channel pot : 0x");
    print_dbg_hex(adc_value_pot);
    print_dbg("\r\n");
#endif
  }
}
