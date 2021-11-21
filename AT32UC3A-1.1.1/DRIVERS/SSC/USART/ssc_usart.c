/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SSC USART example driver.
 *
 * This file defines a useful set of functions for the SSC USART interface on
 * AVR32 devices. The driver handles normal polled usage and direct memory
 * access (PDC) usage.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a SSC module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/* Copyright (c) 2007, Atmel Corporation All rights reserved.
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

#include "ssc_usart.h"

typedef struct ssc_uart_opt_tag {
	int baud_rate;
	char stop_bits;
	char parity;
	char char_len;
	char parity_error;
	char framing_error;
	char overrun_error;
} ssc_uart_opt_t;

/*! Static option struct, used to hold options between ssc_uart_init() and
 *  calls to ssc_uart_putchar() and ssc_uart_getchar()
 */
static ssc_uart_opt_t opt;


/*! \brief Determines whether the argument has even parity
 *
 * This function returns whether the argument has even or odd parity. It
 * returns 1 for even parity.
 *
 * \param ch Word to count bits set in.
 *
 * \retval 0 The count of 1's in ch is odd
 * \retval 1 The count of 1's in ch is even.
 *
 */
static int has_even_parity(unsigned short ch)
{
	int count_1s = 0;

	while( ch > 0 )
	{
		if ( ch & 0x1 )
			count_1s++;

		ch = ch >> 1;
	}

	return (count_1s % 2 == 0);
}

/*! \brief Finds the actual parity bit
 *
 * Finds whether the parity bit is zero or one, based on the current settings
 * and the word to send.
 *
 * \param ch Word to send
 *
 * \retval 0 Parity bit should be 0
 * \retval 1 Parity bit should be 1
 *
 */
static int get_parity_bit(unsigned short ch)
{
	if (opt.parity == SSC_UART_PARITY_NONE)
		return 0;
	else if (opt.parity == SSC_UART_PARITY_SPACE)
		return 0;
	else if (opt.parity == SSC_UART_PARITY_MARK)
		return 1;
	else if (opt.parity == SSC_UART_PARITY_EVEN)
	{ // Parity Even
		if ( has_even_parity(ch) )
			return 0;
		else
			return 1;
	}
	else
	{ // Parity Odd
		if ( has_even_parity(ch) )
			return 1;
		else
			return 0;
	}
}

/*! \brief Returns the stop bit(s) to send
 *
 * Returns the stop bits (0, 1, 11) based on current settings.
 *
 * \return  0b0, 0b1 or 0b11 according to *opts
 */
static int get_stop_bits()
{
	if (opt.stop_bits == SSC_UART_STOPBITS_ONE_AND_HALF)
		return 0x3; // binary: 11
	else if (opt.stop_bits == SSC_UART_STOPBITS_TWO)
		return 0x3; // binary: 11
	else // if (opts->stop_bits == SSC_UART_STOPBITS_ONE)
		return 0x1;
}

/*! \brief Sets the clock divider (DIV-field in CMR-register)
 *
 * This function sets the DIV field in the CMR-register to correct divider
 * so that the divided clock runs at the specified rate.
 *
 * \note Clock divider is always rounded down, this means that the desired clock
 *       will be correct or faster than desired clock rate.
 *
 * \param ssc       Base address of SSC.
 * \param bit_rate  Desired clock rate (bit rate).
 * \param pbaHz     PBA frequency (in Herz)
 */
static int setClockDivider( volatile avr32_ssc_t * ssc,
    unsigned int bit_rate,
    long pbaHz )
{
  /*! \todo check input values */

  ssc->cmr = ((unsigned int) (pbaHz / ( bit_rate*2 ))
    <<AVR32_SSC_CMR_DIV_OFFSET);

  return SSC_UART_SUCCESS;
}


/*! \brief Stops the driver
 *
 * This function stops the driver.
 *
 * \param ssc Pointer to base address of SSC.
 *
 */
static void ssc_uart_reset(volatile avr32_ssc_t  *ssc)
{
	  ssc->cr = AVR32_SSC_CR_SWRST_MASK;
}


int ssc_uart_init(volatile avr32_ssc_t  *ssc, int baud_rate, int stop_bits, int parity_bits,
  int char_len,int loop_back,long pbaHz )
{

	// Check arguments
	if ( baud_rate < 0 )
		return SSC_UART_BAD_ARGS ;

	if ( stop_bits < 0 || stop_bits > SSC_UART_STOPBITS_ONE_AND_HALF )
		return SSC_UART_BAD_ARGS ;

	if ( parity_bits < 0 || parity_bits > SSC_UART_PARITY_MARK )
		return SSC_UART_BAD_ARGS ;

	if ( char_len < 5 || char_len > 9 )
		return SSC_UART_BAD_ARGS ;

	// Store arguments
	opt.baud_rate = baud_rate;
	opt.stop_bits = stop_bits;
	opt.parity= parity_bits;
	opt.char_len = char_len;
	opt.framing_error = 0;
	opt.parity_error = 0;
	opt.overrun_error = 0;

	// Reset SSC:
	ssc_uart_reset(ssc);

	// Set SSC internal clock frequency
	setClockDivider(ssc, baud_rate,pbaHz );

	// Set transmit clock mode:
	//   CKS - use divided clock, CKO - continuous transmit clock,
	//   CKI - shift data on falling clock, START - immediately after data
	//   is inserted in THR. STTDLY, PERIOD - not used.
	ssc->tcmr =
			 (0<<AVR32_SSC_TCMR_CKS_OFFSET)|
			 (2<<AVR32_SSC_TCMR_CKO_OFFSET)|
			 (0<<AVR32_SSC_TCMR_CKG_OFFSET)|
			 (1<<AVR32_SSC_TCMR_CKI_OFFSET)|
			 (0<<AVR32_SSC_TCMR_START_OFFSET)|
			 (0<<AVR32_SSC_TCMR_STTDLY_OFFSET)|
			 (0<<AVR32_SSC_TCMR_PERIOD_OFFSET);

	// Set transmit frame mode:
	//	DATNB - Transfer one word, MSBF - transmit lsb first,
	//	DATDEF - Default to one, FSEDGE, FSDEN, FSOS, FSLEN  - not used,
	//	DATLEN - stop_bits+parity_bits+char_length
	//	(If stopbits=1.5, transmit two)
	ssc->tfmr =
			( ( (stop_bits==SSC_UART_STOPBITS_ONE?1:2) + (parity_bits==0?0:1) + char_len ) << AVR32_SSC_TFMR_DATLEN_OFFSET )|
			(1<<AVR32_SSC_TFMR_DATDEF_OFFSET)|
			(0<<AVR32_SSC_TFMR_MSBF_OFFSET)|
			(1<<AVR32_SSC_TFMR_DATNB_OFFSET)|
			(0<<AVR32_SSC_TFMR_FSLEN_OFFSET)|
			(0<<AVR32_SSC_TFMR_FSOS_OFFSET)|
			(0<<AVR32_SSC_TFMR_FSDEN_OFFSET)|
			(0<<AVR32_SSC_TFMR_FSEDGE_OFFSET);

	// Set receive clock mode:
	//   CKS - use divided clock, CKO - continuous transmit clock,
	//   CKI - shift data on falling edge, START - immediately after data
	//   STOP - wait for new compare 0 after finishing transfer,
	//   STTDLY, PERIOD - not used
	ssc->rcmr =
		  (0<<AVR32_SSC_RCMR_CKS_OFFSET)|
		  (2<<AVR32_SSC_RCMR_CKO_OFFSET)|
		  (1<<AVR32_SSC_RCMR_CKI_OFFSET)|
		  (0<<AVR32_SSC_RCMR_CKG_OFFSET)|
		  (0<<AVR32_SSC_RCMR_START_OFFSET)|
		  (0<<AVR32_SSC_RCMR_STOP_OFFSET)|
		  (0<<AVR32_SSC_RCMR_STTDLY_OFFSET)|
		  (0<<AVR32_SSC_RCMR_PERIOD_OFFSET);

	// Set receive frame mode:
	//   DATLEN - stop_bits+start_bits+parity_bits+char_len,
	//   (if 1.5 stop bits just take one)
	//   LOOP - YES loopback, MSBF - lsb first, DATNB - transmit one per sync,
	//   FSLEN - 1 bit (start bit), FSOS, FSEDGE - not used
    ssc->rfmr =
		      ( ( (stop_bits==0?0:1) +1 +(parity_bits==0?0:1) +char_len ) <<AVR32_SSC_RFMR_DATLEN_OFFSET )|
		      ((loop_back==SSC_UART_LOOPBACK_MODE_ENABLE?1:0)<<AVR32_SSC_RFMR_LOOP_OFFSET)|
		      (0<<AVR32_SSC_RFMR_MSBF_OFFSET)|
		      (1<<AVR32_SSC_RFMR_DATNB_OFFSET)|
		      (0<<AVR32_SSC_RFMR_FSLEN_OFFSET)|
		      (0<<AVR32_SSC_RFMR_FSOS_OFFSET)|
		      (0<<AVR32_SSC_RFMR_FSEDGE_OFFSET);


	// Set compare 0-register
	ssc->rc0r = (0<<AVR32_SSC_RC0R_CP0_OFFSET);

    // Enable receiver and transceiver
	ssc->cr = (1<<AVR32_SSC_CR_TXEN_OFFSET)|(1<<AVR32_SSC_CR_RXEN_OFFSET);
	return SSC_UART_SUCCESS;
}

/*! Validates a received word
 *
 * This function validates a received word, including stopbits, parity bit
 * [and startbit?]. On failure it sets error flags.
 *
 * \param ch Word received
 *
 * \return Non-negative number: Word without start,stop and parity bits
 * \retval SSC_UART_FAILURE Word is invalid. Sets appropriate error flag in
 *                          option struct
 *
 */
static int ssc_uart_validate(unsigned short ch)
{
	int data;
	int parity_bit=0;
	int stop_bits;

	// Get stop bit(s)
	if (opt.parity == SSC_UART_PARITY_NONE)
		stop_bits = ch >> (opt.char_len) ;
	else
		stop_bits = ch >> (opt.char_len+1) ;

	// If the first stop bit is not one, we have a frame error.
	//   Does not check second stop bit, even if n_stop_bits>=2
	if ((stop_bits & 0x1) == 0)
	{
		opt.framing_error = 1;
		return SSC_UART_FAILURE;
	}

	// Gets the parity bit
	parity_bit = ((ch >> (opt.char_len)) & 0x1) ;

	// Gets only useful data
	data = ch & 0xFF;

	// If parity is enabled, and the parity bit differs from what it
   	//   should be, we have a parity error
	if (opt.parity != SSC_UART_PARITY_NONE &&
	  get_parity_bit(data) != parity_bit)
	{
		opt.parity_error = 1;
		return SSC_UART_FAILURE;
	}

	// Return char_len lsb of ch
	 return (ch & ((1<<opt.char_len)-1)) ;
}



int ssc_uart_putchar(volatile avr32_ssc_t  *ssc, unsigned short ch)
{
  	unsigned int word = 0 ;
	int timeout = SSC_UART_DEFAULT_TIMEOUT;

	// Shift data one bit right (because of start-bit) and put in word
	//   The left-most bit is always zero, and is the start-bit
    word = (ch) << 1 ;

	// If parity-bit, find it, shift it past data and start-bit, and add.
	//   Then add stop bits after that.
	if (opt.parity != SSC_UART_PARITY_NONE)
		word |= (get_parity_bit(ch) << (opt.char_len+1)) |
		  (get_stop_bits() << (opt.char_len+2));

	// If no parity bit, add stop bit after data.
	else
		word |= (get_stop_bits() << (opt.char_len+1));

	// wait for Transmit ready
        while( ( ssc->sr & (1<<AVR32_SSC_SR_TXRDY_OFFSET) ) == 0 &&
            timeout > 0 ) {
          timeout--;
        }

	// If timeout, return TIMEOUT
	if (timeout == 0)
		return SSC_UART_TIMEOUT;

	// Write word to transmit holding register
	ssc->thr = word ;

	return SSC_UART_SUCCESS;
}

int ssc_uart_getchar(volatile avr32_ssc_t  *ssc)
{

	// If overrun, set overrun_error flag and return
	if (ssc->SR.ovrun)
	{
		opt.overrun_error = 1;
		return SSC_UART_FAILURE;
	}

	// busy-loop until data arrives
	while (!ssc->SR.rxrdy)
		;

	// validate data and return
	return( ssc_uart_validate(ssc->rhr) ) ;

}


void ssc_uart_reset_status(volatile avr32_ssc_t  * ssc)
{
	opt.parity_error = 0;
	opt.framing_error = 0;
	opt.overrun_error = 0;
}


int ssc_uart_parity_error(volatile avr32_ssc_t  * ssc)
{
	return opt.parity_error;
}


int ssc_uart_framing_error(volatile avr32_ssc_t  * ssc)
{
	return opt.framing_error;
}


int ssc_uart_overrun_error(volatile avr32_ssc_t  * ssc)
{
	return opt.overrun_error;
}
