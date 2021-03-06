/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief File System access example application.
 *
 * This file gives an example of using the FS access, through POSIX commands.
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 *                       The example is written for UC3 and EVK1100.
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
 * This documents data structures, functions, variables, defines, enums, and
 * typedefs in the software. It also gives an example of the usage of the
 * FAT filesystem through the dataflash memory AT45DBX & SPI driver.
 *
 * The example lets you issue the following commands on RS232 using POSIX interfaces (open, read, write, close):
 * - read a file from the memory files system;
 * - concatenate a file to an existing one;
 * - copy the content of a file to a new one.
 * 
 * Note: This example assumes that the dataflash memory contains a file in the
 *       root directory (and of course that the memory filesystem is a
 *       FAT12/16/32).
 *
 * \section files Main Files
 * - file.c : functions for file accesses
 * - file.h : headers for file accesses
 * - fat.c : functions for FAT accesses
 * - fat.h : headers for FAT accesses
 * - fat_unusual.c : add-ons functions for FAT accesses
 * - fs_com.h : structures and defines for file system accesses
 * - navigation.c : functions for navigators accesses
 * - navigation.h : headers for navigators accesses
 * - fsaccess.c : functions for POSIX interfaces
 * - fsaccess.h : headers for POSIX interfaces
 * - ctrl_access.c : functions for memory control access interfaces
 * - ctrl_access.h : headers for memory control access interfaces
 * - fsaccess_example.c : FAT example
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with an SPI module can be used. This example has been tested
 * with the following setup:<BR>
 * - EVK1100 evaluation kit
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


#include <string.h>
#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "print_funcs.h"
#include "pm.h"
#include "gpio.h"
#include "usart.h"
#include "spi.h"
#include "conf_at45dbx.h"
#include "fsaccess.h"


// display a welcome message
#define MSG_WELCOME "\r\n ---------- Welcome to FSACCESS Example ---------- \r\n"
#define MSG_FILENAME "Enter the filename : "
#define MSG_MODE     "\r\nWhat do you want to do ?\r\n\t1 - Read file\r\n\t2 - Concatenate a file to an existing one\r\n\t3 - Copy the content of a file to a new one\r\nenter your choice : "


// display test result
#define TEST_SUCCESS "\t[PASS]\n"
#define TEST_FAIL    "\t[FAIL]\n"

#define CR                    '\r'
#define LF                    '\n'
#define CTRL_Q                0x11
#define CTRL_C                0x03
#define BKSPACE_CHAR          '\b'
#define ABORT_CHAR            CTRL_C
#define QUIT_APPEND           CTRL_Q


#define NB_SECTOR_TO_SEND    4


int fsaccess_example_get_mode (void);
void fsaccess_example_get_filename (char * buf);
int fsaccess_example_read(char * path);
int fsaccess_example_write(char * source, char * destination, int flags);




/*! \brief Initialize daflash memory AT45DBX resources: GPIO, SPI and AT45DBX
 *
 */
static void at45dbx_resources_init(void)
{
  static const gpio_map_t AT45DBX_SPI_GPIO_MAP =
  {
    {AT45DBX_SPI_SCK_PIN,          AT45DBX_SPI_SCK_FUNCTION         },  // SPI Clock.
    {AT45DBX_SPI_MISO_PIN,         AT45DBX_SPI_MISO_FUNCTION        },  // MISO.
    {AT45DBX_SPI_MOSI_PIN,         AT45DBX_SPI_MOSI_FUNCTION        },  // MOSI.
#define AT45DBX_ENABLE_NPCS_PIN(NPCS, unused) \
    {AT45DBX_SPI_NPCS##NPCS##_PIN, AT45DBX_SPI_NPCS##NPCS##_FUNCTION},  // Chip Select NPCS.
    MREPEAT(AT45DBX_MEM_CNT, AT45DBX_ENABLE_NPCS_PIN, ~)
#undef AT45DBX_ENABLE_NPCS_PIN
  };

  // SPI options.
  spi_options_t spiOptions =
  {
    .reg          = 0,
    .baudrate     = AT45DBX_SPI_MASTER_SPEED, // Defined in conf_at45dbx.h.
    .bits         = AT45DBX_SPI_BITS,         // Defined in conf_at45dbx.h.
    .spck_delay   = 0,
    .trans_delay  = 0,
    .stay_act     = 1,
    .spi_mode     = 0,
    .fdiv         = 0,
    .modfdis      = 1
  };

  // Assign I/Os to SPI.
  gpio_enable_module(AT45DBX_SPI_GPIO_MAP,
                     sizeof(AT45DBX_SPI_GPIO_MAP) / sizeof(AT45DBX_SPI_GPIO_MAP[0]));

  // Initialize as master.
  spi_initMaster(AT45DBX_SPI, &spiOptions);

  // Set selection mode: variable_ps, pcs_decode, delay.
  spi_selectionMode(AT45DBX_SPI, 0, 0, 0);

  // Enable SPI.
  spi_enable(AT45DBX_SPI);

  // Initialize data flash with SPI clock Osc0.
  at45dbx_init(spiOptions, FOSC0);
}


/*! \brief Main function, execution starts here.
 *  RS232 is used to input/output information.
 *  The example lets you issue the following commands on RS232 using POSIX interfaces (open, read, write, close):
 *  - read a file from the memory files system;
 *  - concatenate a file to an existing one;
 *  - copy the content of a file to a new one.
 *
 * Connect USART_1 to your serial port via a standard RS-232 D-SUB9 cable
 * Set the following settings in your terminal of choice: 57600 8N1
 * 
 */
int main(void)
{
char filename1[90];
char filename2[90];

  // switch to external oscillator 0
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // initialize RS232 debug text output
  init_dbg_rs232(FOSC0);

  // initialize AT45DBX resources: GPIO, SPI and AT45DBX
  at45dbx_resources_init();

  // initialize FSACCESS mutex and navigators
  b_fsaccess_init();

  // Try to init data flash
  if (at45dbx_mem_check())
  {
    // display welcome message
    print_dbg(MSG_WELCOME);
  }
  else
  {
    // display error message
    print_dbg("Initialization failed\r\n");
    return (-1);
  }

  while (TRUE)
  {
    // check the user command
    switch (fsaccess_example_get_mode())
    {
    case '1':
      // Wait for a filename
      fsaccess_example_get_filename(filename1);
      // Read it and display the content
      fsaccess_example_read(filename1);
    break;
    case '2':
      // Wait for source filename
      print_dbg("Source : ");
      fsaccess_example_get_filename(filename1);
      // Wait for destination filename
      print_dbg("Destination : ");
      fsaccess_example_get_filename(filename2);
      // Write from source to destination (append to the existing file)
      fsaccess_example_write(filename1, filename2, O_APPEND);
    break;
    case '3':
      // Wait for source filename
      print_dbg("Source : ");
      fsaccess_example_get_filename(filename1);
      // Wait for destination filename
      print_dbg("Destination : ");
      fsaccess_example_get_filename(filename2);
      // Write from source to destination (append to the unexisting file)
      fsaccess_example_write(filename1, filename2, (O_CREAT | O_APPEND));
    break;
    default:
      // Display error message
      print_dbg("Wrong choice\r\n");
    break;
    }
  }
}

/*! \brief Wait for user to select the mode he wants to test
 */
int fsaccess_example_get_mode (void)
{
int c;

  // Display asking message
  print_dbg(MSG_MODE);
  while (TRUE)
  {
    // If something new in the USART :
    if (usart_read_char(DBG_USART, &c) == USART_SUCCESS)
    {
      // Echo char
      print_dbg_char(c);
      // Add LF
      print_dbg_char(LF);
      return (c);
    }
  }
}

/*! \brief Wait for user to enter a filename
 */
void fsaccess_example_get_filename (char * buf)
{
unsigned long i_str = 0;
unsigned short file;
int c;

  // Display asking message
  print_dbg(MSG_FILENAME);
  file = FALSE;

  while (file == FALSE)
  {
    // If something new in the USART :
    if (usart_read_char(DBG_USART, &c) == USART_SUCCESS)
    {
      // Regarding the received char :
      switch (c)
      {
      // This is a carriage return
      case CR:
        // Add LF
        print_dbg_char(LF);
        // Add NUL char
        buf[i_str] = '\0';
        // End the loop
        file = TRUE;
        // Reset string length
        i_str = 0;
      break;
      // This is a CTRL+C
      case ABORT_CHAR:
        i_str = 0;
        print_dbg_char(LF);       // Add LF
        print_dbg(MSG_FILENAME);
      break;
      // This is a backspace
      case BKSPACE_CHAR:
        if (i_str > 0)
        {
          // Echo the backspace
          print_dbg_char(c);
          // Echo a space
          print_dbg_char(' ');
          // Echo the backspace
          print_dbg_char(c);
          // Decrease string length
          i_str--;
        }
      break;
      // This is a char
      default:
        // Echo it
        print_dbg_char(c);
        // Append to cmd line and increase string length
        buf[i_str++] = c;
      break;
      }
    }
  }
}

/*! \brief Read a file and display it to USART
 */
int fsaccess_example_read(char * path)
{
char * ptrFile;
int fd, i;
long size;

  // Try to open the file
  if ((fd = open(path, O_RDONLY)) < 0)
  {
    // Display error message
    print_dbg(path);
    print_dbg(" : Open failed\n");
    return (-1);
  }

  // Get file size
  size = fsaccess_file_get_size(fd);
  // Display file size
  print_dbg("File size: ");
  print_dbg_ulong(size);
  print_dbg_char(LF);

  // Allocate a buffer
  ptrFile = malloc((NB_SECTOR_TO_SEND * FS_SIZE_OF_SECTOR) + 1);

  // Allocation fails
  if (ptrFile == NULL)
  {
    // Display error message
    print_dbg("Malloc failed\n");
  }
  else
  {
    // Try to perform a single access
    if ( size < (NB_SECTOR_TO_SEND * FS_SIZE_OF_SECTOR) )
    {
      if( read(fd, ptrFile, size) != size)
      {
         // Display error message
         print_dbg("Reading entire file failed\n");
      }
       else
       {
         // Add a null terminating char
         ptrFile[size] = '\0';
         // Display the buffer to user
         print_dbg(ptrFile);
       }
    }
    else
    {
      // Try to send the biggest frame contained in the file
      for (i = NB_SECTOR_TO_SEND ; i > 0 ; i--)
      {
        // Get sectors of maximum size
        while(size > i * FS_SIZE_OF_SECTOR)
        {
          // Get the data from file
          if( read(fd, ptrFile, i * FS_SIZE_OF_SECTOR) !=  i * FS_SIZE_OF_SECTOR)
          {
            // Display error message
            print_dbg("Reading file block failed\n");
            // Close file
            close(fd);
            return (-1);
          }
          // Add a null terminating character
          ptrFile[i * FS_SIZE_OF_SECTOR] = '\0';
          // Display buffer content to user
          print_dbg(ptrFile);
          // Decrease remaining size
          size -= (i * FS_SIZE_OF_SECTOR);
        }
      }
      // Finish with the few data remaining (less than 1 sector)
      if ( size > 0 )
      {
        // Get the data from filesystem
        if( read(fd, ptrFile, size) != size)
        {
          // Display error message
          print_dbg("Reading file end failed\n");
          // Close file
          close(fd);
          return (-1);
        }
        else
        {
          // Add a null terminating char
          ptrFile[size] = '\0';
          // Display the buffer to user
          print_dbg(ptrFile);
        }
      }
    }
    // Free the buffer
    free(ptrFile);
  }
  // Close file
  close(fd);
  return (0);
}

/*! \brief Copy a file to another one
 */
int fsaccess_example_write(char * source, char * destination, int flags)
{
char * ptrFile;
int fd1, fd2, i;
long size;
int ErrorCode = -1;

  if ((fd1 = open(source, O_RDONLY)) < 0)
  {
    // Display error message
    print_dbg(source);
    print_dbg(": Open failed\n");
    return (ErrorCode);
  }
  if ((fd2 = open(destination, flags)) < 0)
  {
    // Display error message
    print_dbg(destination);
    print_dbg(": Open failed\n");
    return (ErrorCode);
  }

  // Get file size
  size = fsaccess_file_get_size(fd1);
  // Display file size
  print_dbg("Copying ");
  print_dbg_ulong(size);
  print_dbg(" Bytes\n");

  // Allocate a buffer
  ptrFile = malloc(NB_SECTOR_TO_SEND * FS_SIZE_OF_SECTOR);

  // Allocation fails
  if (ptrFile == NULL)
  {
    // Display error message
    print_dbg("Malloc failed\n");
    return (ErrorCode);
  }
  else
  {
    if ( size <= (NB_SECTOR_TO_SEND * FS_SIZE_OF_SECTOR) )
    {
      if ( read(fd1, ptrFile, size) != size )
      {
        // Display error message
        print_dbg("Reading entire file failed\n");
        // Escape
        goto close_end;
      }
      if ( write(fd2, ptrFile, size) != size )
      {
        // Display error message
        print_dbg("Writing entire file failed\n");
        // Escape
        goto close_end;
      }
    }
    else
    {
      // Try to send the biggest frame contained in the file
      for (i = NB_SECTOR_TO_SEND ; i > 0 ; i--)
      {
        // Get sectors of maximum size
        while(size > i * FS_SIZE_OF_SECTOR)
        {
          // Clear previous buffer
          memset(ptrFile, 0, NB_SECTOR_TO_SEND * FS_SIZE_OF_SECTOR);
          // Read the data from source file
          if( read(fd1, ptrFile, i * FS_SIZE_OF_SECTOR) !=  i * FS_SIZE_OF_SECTOR)
          {
            print_dbg("Reading file block failed\n");
            // Escape
            goto close_end;
          }
          // Write the data to destination file
          if ( write(fd2, ptrFile, i * FS_SIZE_OF_SECTOR) != i * FS_SIZE_OF_SECTOR )
          {
            print_dbg("Writing file block failed\n");
            // Escape
            goto close_end;
          }
          // Decrease remaining size
          size -= (i * FS_SIZE_OF_SECTOR);
        }
      }
      // Finish with the few data remaining (less than 1 sector)
      if ( size > 0 )
      {
      	// Get the data from filesystem
        if( read(fd1, ptrFile, size) !=  size)
        {
          print_dbg("Reading file end failed\n");
          // Escape
          goto close_end;
        }
        if ( write(fd2, ptrFile, size) != size )
        {
          print_dbg("Writing file end failed\n");
          // Escape
          goto close_end;
        }
      }
    }
    // No error occurs here
    ErrorCode = 0;
  }

close_end:
  // Free the buffer
  free(ptrFile);
  // Close files
  close(fd1);
  close(fd2);
  return (ErrorCode);
}
