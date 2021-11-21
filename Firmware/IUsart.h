/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef IUsart_h_
#define IUsart_h_

/** \file Callback-driven USART interface. */

#if defined(__cplusplus)
extern "C" {
#endif

/** Receive callback. Argument is a character received. */
typedef void (*IUsart_RxCallback)(const char);

typedef enum {
	/** USART_0 */
	IUsart0	= 0,
	/** USART_1 */
	IUsart1	= 1
} IUsart;

/** IUsart mode. */
typedef enum {
	/** Conventional RS232. */
	IUsart_RS232,
	/** RS485 */
	IUsart_RS485
} IUsart_Mode;

/** Initialize Usart.
\param[in]	UsartNr				Number of USART.
\param[in]	Mode				Either RS232 or RS485.
\param[in]	InterruptPriority	Interrupt priority, INT0 - INT3.
\param[in]	BaudRate			Baud rate.
\param[in]	RxCallback			Function to call when character is received.
 */
extern void IUsart_Init(
	const IUsart			UsartNr,
	const IUsart_Mode		Mode,
	const unsigned int		InterruptPriority,
	const unsigned int		BaudRate,
	const IUsart_RxCallback	RxCallback
);

/** Write one character to Usart. NB! This function has no queue and blocks on write! */
extern void IUsart_Write(
	IUsart					UsartNr,
	const char				C
);

#if defined(__cplusplus)
}
#endif


#endif /* IUsart_h_ */

