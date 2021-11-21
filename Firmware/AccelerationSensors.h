/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef AccelerationSensors_h_
#define AccelerationSensors_h_

#include "LoggerIO.h"
#include "CircularBuffer.h"

typedef struct {
	unsigned char	buffer[LoggerIO::SENSORS_BUFFER_SIZE + 8];
	uint16_t		rxtick[LoggerIO::SENSORS_BUFFER_SIZE + 8];
	uint32_t		tick;
	uint16_t		count;
} SENSORS_RXBUFFER;

/** Receive queue. */
extern CircularBuffer<SENSORS_RXBUFFER>	AccelerationSensors_RxQueue;

/** Packet converter. */
extern void
AccelerationSensors_Convert(
	LoggerIO::SENSORS&		dst,
	const SENSORS_RXBUFFER&	src
);

/** Initialize acceleration sensors module. */
extern void
AccelerationSensors_Init(
	const unsigned int	sampling_rate
);

/** Query the tick :) */
extern unsigned int
AccelerationSensors_GetTick();

/** Is packet within current limits? */
extern bool
AccelerationSensors_PacketWithinLimits(
	const LoggerIO::SENSORS&	packet
);

/** Process display data :) */
extern void
AccelerationSensors_Display_Process(void);

#endif /*AccelerationSensors_h_ */
