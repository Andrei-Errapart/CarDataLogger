/**
vim: ts=4
vim: shiftwidth=4
*/
#include <stdio.h>
#include <string.h>

#include "LoggerConfig.h"

#include "Gps.h"
#include "Display.h"
#include "Utils.h"
#include "IUsart.h"
#include "IClock.h"	// delay_ms.
#include "tprintf.h"
#include "AccelerationSensors.h"	// AccelerationSensors_GetTick()
#include "project.h"

// #define	ROUNDS_PER_SECOND			F_CPU
#define	TIMEOUT_ROUNDS				(2 * LoggerConfig::SamplingFrequency)

#define	FIELD_MAX_COUNT				20
#define	FIELD_MAX_LENGTH			20

CircularBuffer<LoggerIO::GPS>	Gps_RxQueue(0, 10);

typedef enum {
	PHASE_LOOK_FOR_FIRST,
	PHASE_PARSE_BODY,
	PHASE_CHECKSUM_CHAR1,
	PHASE_CHECKSUM_CHAR2
} PHASE;

/** Round of last received character. */
static unsigned int		last_rx_round		= 0;
/** Round of last fix. */
static unsigned int		last_fix_round		= 0;
/** Round of last correct PGRMF message. */
static unsigned int		last_pgrmf_round	= 0;

static PHASE			phase = PHASE_LOOK_FOR_FIRST;

// static LoggerIO::GPS	nmea_buf;
static unsigned int		nmea_data_size = 0;
static unsigned char	nmea_checksum;
static unsigned char	nmea_rcvd_checksum;

/** Sentence fields. */
static unsigned char	field[FIELD_MAX_COUNT][FIELD_MAX_LENGTH+1];
static unsigned int		field_index = 0;
static unsigned int		field_char_index = 0;

/** See $PGRMF sentence description in Garmin GPS18 manual for information. */
#define	PGRMFINDEX_DATEFIX				3
#define	PGRMFINDEX_TIMEFIX				4
#define	PGRMFINDEX_LATITUDE				6
#define	PGRMFINDEX_LATITUDE_HEMISPHERE	7
#define	PGRMFINDEX_LONGITUDE			8
#define	PGRMFINDEX_LONGITUDE_HEMISPHERE	9
#define	PGRMFINDEX_FIX					11
#define	PGRMFINDEX_SPEEDOVERGROUND		12
#define	PGRMFINDEX_COURSEOVERGROUND		13

#define	FIX_NONE	'0'
#define	FIX_2D		'1'
#define	FIX_3D		'2'

//*******************************************************************
static int hex_to_bin(char ch)
{
	if (isxdigit(ch))
	{
		if(isdigit(ch))
			ch -= '0';
		else if(islower(ch))
			ch -= 'a' - 10;
		else 
			ch -= 'A' - 10;
	}
	else return -1;
	
	return ch;
}

//*******************************************************************
#define	append_nmea_buf(c)										\
	do {														\
		if (nmea_data_size < sizeof(nmea_buf.NmeaLine)) {		\
			nmea_buf.NmeaLine[nmea_data_size++] = c;			\
		}														\
	} while (0)

//*******************************************************************
static void handle_NMEA_char(const char ch)
{
	LoggerIO::GPS&	nmea_buf = Gps_RxQueue.Poke();
	last_rx_round = AccelerationSensors_GetTick();
	
	switch (phase) {
	case PHASE_LOOK_FOR_FIRST:
		if (ch == '$') {
			nmea_data_size = 0;
			nmea_checksum = 0;
			field_index = 0;
			field_char_index = 0;
			phase = PHASE_PARSE_BODY;
			nmea_buf.Header.Type		= LoggerIO::TYPE_GPS;
			nmea_buf.Header.TotalSize	= sizeof(LoggerIO::GPS);
			nmea_buf.Header.Tick		= AccelerationSensors_GetTick();
			for (unsigned int i=0; i<FIELD_MAX_COUNT; ++i) {
				field[i][0] = 0;
			}
		}
		break;
	case PHASE_PARSE_BODY:
		if (ch == '*') {
			phase = PHASE_CHECKSUM_CHAR1;
			append_nmea_buf(ch);
			break;
		} else {
			if (nmea_data_size < sizeof(nmea_buf.NmeaLine)) {
				nmea_buf.NmeaLine[nmea_data_size++] = ch;
				nmea_checksum = nmea_checksum ^ ch;
				if (field_index < FIELD_MAX_COUNT) {
					if (ch == ',') {
						field[field_index][field_char_index] = 0;
						++field_index;
						field_char_index = 0;
					} else {
						if (field_char_index < FIELD_MAX_LENGTH) {
							field[field_index][field_char_index] = ch;
							++field_char_index;
						}
					}
				}
			} else {
				phase = PHASE_LOOK_FOR_FIRST;
			}
		}
		break;
	case PHASE_CHECKSUM_CHAR1:
		{
			append_nmea_buf(ch);
			const int x = hex_to_bin(ch);
			if (x >= 0) {
				nmea_rcvd_checksum = (unsigned char) x << 4;			
				phase = PHASE_CHECKSUM_CHAR2;
			} else {
				phase = PHASE_LOOK_FOR_FIRST;
			}
		}
		break;
	case PHASE_CHECKSUM_CHAR2:
		{
			append_nmea_buf(ch);
			const int x = hex_to_bin(ch);
			nmea_buf.NmeaLine[nmea_data_size] = 0;

			if (x >= 0) {
				nmea_rcvd_checksum |= (unsigned char) x;
				if (nmea_rcvd_checksum==nmea_checksum) {
					// Push it anyway :)
					Gps_RxQueue.Push();

					// Update display if possible.
					if (strncmp(reinterpret_cast<const char*>(nmea_buf.NmeaLine), "PGRMF", 5)==0) {
						const char	fix = field[PGRMFINDEX_FIX][0];

						last_pgrmf_round = AccelerationSensors_GetTick();
						if (fix==FIX_2D || fix==FIX_3D) {
							last_fix_round = AccelerationSensors_GetTick();
						}
					}
				}
			} else {
				phase = PHASE_LOOK_FOR_FIRST;
			}
			phase = PHASE_LOOK_FOR_FIRST;
		}
		break;
	default:
		phase = PHASE_LOOK_FOR_FIRST;
		break;
	}
}

//*******************************************************************
void
Gps_Init(	const unsigned int	baud_rate)
{
	tprintf("GPS...");

	const unsigned int	timeout_round = AccelerationSensors_GetTick() - TIMEOUT_ROUNDS - 1;
	last_rx_round		= timeout_round;
	last_fix_round		= timeout_round;
	last_pgrmf_round	= timeout_round;

	delay_ms(5);
	IUsart_Init(IUsart0, IUsart_RS232, INT0, baud_rate, handle_NMEA_char);
	delay_ms(5);

	tprintf(" done.\n");
}

//*******************************************************************
void
Gps_Display_Process()
{
	const unsigned int	current_round = AccelerationSensors_GetTick();
	const bool	has_rx = is_between(current_round,		last_rx_round,		last_rx_round + TIMEOUT_ROUNDS);
	const bool	has_fix = is_between(current_round,		last_fix_round,		last_fix_round + TIMEOUT_ROUNDS);
	const bool	has_pgrmf = is_between(current_round,	last_pgrmf_round,	last_pgrmf_round + TIMEOUT_ROUNDS);

	if (has_rx) {
		if (has_fix) {
			if (has_pgrmf) {
				Display_Gps_Line("GPS: OK.");
			} else {
				Display_Gps_Line("GPS: Missing PGRMF.");
			}
		} else {
			Display_Gps_Line("GPS: No satellites.");
		}
	} else {
		Display_Gps_Line("GPS: Disconnected.");
	}
}

