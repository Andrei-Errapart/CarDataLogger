/*
vim: ts=4
vim: shiftwidth=4
*/
#include "LoggerConfig.h"
#include "AccelerationSensors.h"
#include "Display.h"
#include "Utils.h"
#include "IUsart.h"
#include "ITimer.h"
#include "IClock.h"
#include "tprintf.h"
#include "project.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SENSORS_BAUD_RATE 		500000
#define	SENSORS_QUERY			0xAA

#define	BUMP_TIMEOUT_ROUNDS		(LoggerConfig::SamplingFrequency)

CircularBuffer<SENSORS_RXBUFFER>	AccelerationSensors_RxQueue(0, 10);

/** Round number. */
static volatile unsigned int		current_round = 0;
/** Number of CPU ticks per round. */
static unsigned int					cputicks_per_round = F_CPU / LoggerConfig::DEFAULT_SAMPLING_FREQUENCY;
/** Start time of the round, in ticks. */
static unsigned int					round_start_ticks = 0;

//*******************************************************************
static void
AccelerationSensors_RxChar(
	const char	c
)
{
	SENSORS_RXBUFFER&	el = AccelerationSensors_RxQueue.Poke();
	uint16_t&			el_count = el.count;
	if (el_count < sizeof(el.buffer)) {
		el.buffer[el_count] = c;
		el.rxtick[el_count] = GetTSC() - round_start_ticks;
		++el_count;
	}
}


//*******************************************************************
/** Decode character receive time into packet and byte indices.
 * \param[out]	packet_index	Packet index, 0...6
 * \param[out]	byte_index		Byte index, 0...3
 * \return		true iff the values are within range, false otherwise.
 */
static inline bool
decode_rxtick(
	unsigned int&		packet_index,
	unsigned int&		byte_index,
	const int			rx_time	
	)
{
	uint16_t	temp_packet;
	uint16_t	temp_byte;
	temp_packet = rx_time + LoggerConfig::SensorsTicksPacket/2 - LoggerConfig::SensorsTicksOffset;
	packet_index = temp_packet / LoggerConfig::SensorsTicksPacket;
	if (packet_index < LoggerIO::SENSORS_MAX_PACKETS) {
		temp_byte = rx_time  + LoggerConfig::SensorsTicksByte/2 - packet_index*LoggerConfig::SensorsTicksPacket - LoggerConfig::SensorsTicksOffset;
		byte_index = temp_byte / LoggerConfig::SensorsTicksByte;
		return byte_index < LoggerIO::SENSORS_PACKET_SIZE;
	} else {
		return false;
	}
}

//*******************************************************************
static inline bool
bytes_in_line(
	const uint16_t*		packet_timings
)
{
	const uint16_t	t0 = packet_timings[0];
	for (unsigned int i=1; i<LoggerIO::SENSORS_PACKET_SIZE; ++i) {
		const unsigned int	index = (packet_timings[i] + LoggerConfig::SensorsTicksByte/2 - t0) / LoggerConfig::SensorsTicksByte;
		if (index != i) {
			return false;
		}
	}
	return true;
}

//*******************************************************************
static void
DecodeData(
	const unsigned char*	buf,
	unsigned int&			x_axis, 
	unsigned int&			y_axis,
	unsigned int&			z_axis
)
{
	unsigned char decoded_data[8], *p;
	
	p = decoded_data;
	*p++ = (unsigned char)(unsigned int)buf[0];
	*p++ = (unsigned char)((unsigned int)buf[1] & 0x03);
	
	*p++ = (unsigned char)((((unsigned int)buf[1] & 0xfc) >> 2) | 
			(((unsigned int)buf[2] & 0x0f) << 6));
	*p++ = (unsigned char)(((unsigned int)buf[2] & 0x0f) >> 2);
	
	*p++ = (unsigned char)(((unsigned int)buf[2] & 0xf0 >> 4) |
			(((unsigned int)buf[3] & 0x3f) << 4));
	*p++ = (unsigned char)(((unsigned int)buf[3] & 0x3f) >> 4);
	
	x_axis = (unsigned int) decoded_data[0] | (unsigned int) decoded_data[1] << 8; 
	y_axis = (unsigned int) decoded_data[2] | (unsigned int) decoded_data[3] << 8;
	z_axis = (unsigned int) decoded_data[4] | (unsigned int) decoded_data[5] << 8;
}

//*******************************************************************
static void
timer_sampling( void )
{
	// 1. Push, if any.
	if (current_round > 0) {
		AccelerationSensors_RxQueue.Push();
	}

	// 2. Increment current round counter.
	{
		++current_round;
		// sometimes the tick gets lost :(
		const unsigned int	dt = GetTSC() - round_start_ticks;
		if (dt > (3*cputicks_per_round/2)) {
			++current_round;
		}
	}

	// 3. Start it again.
	SENSORS_RXBUFFER&	el = AccelerationSensors_RxQueue.Poke();
	el.tick = current_round;
	el.count = 0;
	round_start_ticks = GetTSC();
	IUsart_Write(IUsart1, SENSORS_QUERY);
}

//*******************************************************************
void
AccelerationSensors_Convert(
	LoggerIO::SENSORS&		dst,
	const SENSORS_RXBUFFER&	src
)
{
	const unsigned int	rx_count = src.count;
	unsigned int		byte_index = 0;
	unsigned int		packet_index = 0;

	// Update header.
	dst.Header.Type			= LoggerIO::TYPE_SENSORS;
	dst.Header.TotalSize	= sizeof(LoggerIO::SENSORS);
	dst.Header.Tick			= src.tick;
	memset(dst.Readings, 0, sizeof(dst.Readings));

	// Parse incoming data.
	for (unsigned int i=0; i+LoggerIO::SENSORS_PACKET_SIZE<=rx_count; ++i) {
		if (decode_rxtick(packet_index, byte_index, src.rxtick[i]) && byte_index==0 && bytes_in_line(src.rxtick+i)) {
			// Copy packet data.
			const unsigned int	dst_offset = packet_index*LoggerIO::SENSORS_PACKET_SIZE + byte_index;
			memcpy(dst.Readings+dst_offset, src.buffer+i, LoggerIO::SENSORS_PACKET_SIZE);
		}
	}
}

//*******************************************************************
/** State of sensors. */
typedef struct {
	unsigned int	last_read_round;	/** Last read round. */
	unsigned int	last_min_round;		/** last round reading was under the minimum. */
	unsigned int	last_max_round;		/** last round reading was over the maximum. */
} SENSOR_STATE;

static SENSOR_STATE	sensor_state[LoggerIO::SENSORS_MAX_PACKETS];

//*******************************************************************
void
AccelerationSensors_Init(
	const unsigned int	sampling_rate
)
{
	tprintf("Acceleration sensors...");
	memset(sensor_state, 0, sizeof(sensor_state));

	cputicks_per_round = F_CPU / sampling_rate;
	round_start_ticks = GetTSC();

	IUsart_Init(IUsart1, IUsart_RS485, INT1, SENSORS_BAUD_RATE, AccelerationSensors_RxChar);
	ITimer_Init(ITimer0, INT0, 1000000 / sampling_rate, timer_sampling);
	tprintf(" done.\n");
}

//*******************************************************************
unsigned int
AccelerationSensors_GetTick()
{
	return current_round;
}

//*******************************************************************
bool
AccelerationSensors_PacketWithinLimits(
	const LoggerIO::SENSORS&	packet
)
{
	unsigned int	x;
	unsigned int	y;
	unsigned int	z;

	for (unsigned int i=0; i<LoggerIO::SENSORS_MAX_PACKETS; ++i) {
		const LoggerConfig::AccelerationMinMax&	limits = LoggerConfig::LimitsAcceleration[i];
		DecodeData(packet.Readings + i*LoggerIO::SENSORS_PACKET_SIZE, x, y, z);
		if (x!=0 && y!=0 && z!=0) {
			const bool	ok =
				x>=limits.MinX && x<=limits.MaxX &&
				y>=limits.MinY && y<=limits.MaxY &&
				z>=limits.MinZ && z<=limits.MaxZ;
			if (!ok) {
				return false;
			}
		}
	}
	return true;
}

//*******************************************************************

static const char	timer_scroll[] = "* ";

//*******************************************************************
void
AccelerationSensors_Display_Process(void)
{
	const unsigned int	rxqueue_size = AccelerationSensors_RxQueue.Size();
	if (rxqueue_size>=1) {
		const SENSORS_RXBUFFER&	rxpacket = AccelerationSensors_RxQueue.Peek(rxqueue_size - 1);
		const unsigned int		rx_count = rxpacket.count;
		const unsigned int		rx_tick = rxpacket.tick;
		unsigned int			byte_index = 0;
		unsigned int			packet_index = 0;
		char					xbuf[34];
		unsigned int			x,y,z;
		char*					xptr = xbuf;

#if defined(TRACE_SENSORS_TIMING)
		tprintf("i: ");
#endif
		for (unsigned int i=0; i<rx_count; ++i) {
			if (decode_rxtick(packet_index, byte_index, rxpacket.rxtick[i])
			 && byte_index==0
			 && i+LoggerIO::SENSORS_PACKET_SIZE<=rx_count
			 && bytes_in_line(rxpacket.rxtick+i)) {
				const LoggerConfig::AccelerationMinMax&	limits = LoggerConfig::LimitsAcceleration[packet_index];
				SENSOR_STATE&							st = sensor_state[packet_index];

				DecodeData(rxpacket.buffer + i, x, y, z);

				// Update sensor state.
				st.last_read_round = rx_tick;
				if (x>limits.MaxX || y>limits.MaxY || z>limits.MaxZ) {
					st.last_max_round = rx_tick;
				}
				if (x<limits.MinX || y<limits.MinY || z<limits.MinZ) {
					st.last_min_round = rx_tick;
				}
			}
#if defined(TRACE_SENSORS_TIMING)
			tprintf("%d ", (int)rxpacket.rxtick[i]);
#endif
		}
#if defined(TRACE_SENSORS_TIMING)
		tprintf("\n");
#endif

		// Update display.
		*xptr = timer_scroll[(rx_tick >> 10) & 0x01];
		++xptr;

		*xptr = ' ';
		++xptr;

		for (unsigned int i=0; i<LoggerIO::SENSORS_MAX_PACKETS; ++i) {
			SENSOR_STATE&	st = sensor_state[i];

			if (st.last_read_round==rx_tick) {
				const bool		is_max = is_between(rx_tick, st.last_max_round, st.last_max_round + BUMP_TIMEOUT_ROUNDS);
				const bool		is_min = is_between(rx_tick, st.last_min_round, st.last_min_round + BUMP_TIMEOUT_ROUNDS);

				*xptr = (is_max && is_min) ? '*' : (is_max ? '+' : (is_min ? '-' : ' '));
				++xptr;

				*xptr = '1' + i;
				++xptr;
			} else {
				*xptr = ' ';
				++xptr;
				*xptr = ' ';
				++xptr;
			}
		}
		*xptr = 0;
		Display_AccelerationSensors(xbuf);
	}
}

