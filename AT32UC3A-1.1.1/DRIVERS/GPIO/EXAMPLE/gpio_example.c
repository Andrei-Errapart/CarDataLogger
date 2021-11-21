/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief GPIO example application for AVR32 UC3.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with GPIO.
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
 * This is the documentation for the data structures, functions, variables,
 * defines, enums, and typedefs for the GPIO driver.
 *
 * The General Purpose Input/Output manages the I/O pins of the microcontroller. Each I/O line
 * may be dedicated as a general-purpose I/O or be assigned to a function of an embedded peripheral.
 * This assures effective optimization of the pins of a product.
 *
 * \image html gpio.jpg "Overview of the GPIO System (extracted from datasheet)"
 *
 * The given example covers various use of the GPIO controller and demonstrates
 * different GPIO functionality. It uses a LED and the push button PB0.
 *
 * \section files Main Files
 * - gpio.c: GPIO driver;
 * - gpio.h: GPIO driver header file;
 * - gpio_example.c: GPIO example application.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a GPIO module can be used. This example has been tested
 * with the following setup:<BR>
 * <ul>
 *   <li>EVK1100 or EVK1101 evaluation kit.
 * </ul>
 *
 * \section setupinfo Setup Information
 * <BR>CPU speed: <i> Internal RC oscillator (about 115200 Hz). </i>
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */


#include "compiler.h"
#include "gpio.h"
#include "board.h"


#define GPIO_PIN_EXAMPLE_1  LED2_GPIO
#define GPIO_PIN_EXAMPLE_2  LED3_GPIO
#define GPIO_PIN_EXAMPLE_3  GPIO_PUSH_BUTTON_0


/*! \brief This is an example of how to access the GPIO pin, the first part of the main() explains how
 * to access GPIO register.
 * It is also possible to use the GPIO driver gpio.c.
 * The second part of the main uses the gpio.c driver to set and clear the pin GPIO_PIN_EXAMPLE.
 */
int main(void)
{
  U32 state = 0;
  U32 i;

  gpio_enable_pin_glitch_filter(GPIO_PIN_EXAMPLE_3);
  while (1)
  {
    switch (state)
    {
    case 0:
      // Access with GPIO driver gpio.c with clear and set access.
      gpio_clr_gpio_pin(GPIO_PIN_EXAMPLE_1);
      state++;
      break;

    case 1:
      gpio_set_gpio_pin(GPIO_PIN_EXAMPLE_1);
      state++;
      break;

    case 2:
      // Note that it is also possible to use the GPIO toggle feature.
      gpio_tgl_gpio_pin(GPIO_PIN_EXAMPLE_1);
      state++;
      break;

    case 3:
    default:
      gpio_tgl_gpio_pin(GPIO_PIN_EXAMPLE_1);
      state = 0;
      break;
    }

    // Poll push button value.
    for (i = 0; i < 1000; i += 4)
    {
      if (gpio_get_pin_value(GPIO_PIN_EXAMPLE_3) == 0)
        gpio_clr_gpio_pin(GPIO_PIN_EXAMPLE_2);
      else
        gpio_set_gpio_pin(GPIO_PIN_EXAMPLE_2);
    }
  }
}
