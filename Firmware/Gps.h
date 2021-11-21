/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Gps_h_
#define Gps_h_

#include <CircularBuffer.h>
#include <LoggerIO.h>

/** GPS receive queue. */
extern CircularBuffer<LoggerIO::GPS>	Gps_RxQueue;

/** Initialize GPS USART_0 with given baud rate. */
extern void Gps_Init(
	const unsigned int	baud_rate
);

extern void
Gps_Display_Process(void);

#endif /* Gps_h_ */
