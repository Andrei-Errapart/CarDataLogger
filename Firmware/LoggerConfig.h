/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef LoggerConfig_h_
#define	LoggerConfig_h_

// #include "AccelerationSensors.h"
#include "LoggerIO.h"

#include <Filesystem/FAT16.h>

/** Global variables holding logger configuration. */
namespace LoggerConfig {
	/** Configuration defaults. */
	enum {
		DEFAULT_SENSORS_TICKS_OFFSET	= 3785,
		DEFAULT_SENSORS_TICKS_BYTE		= 1588,
		DEFAULT_SENSORS_TICKS_PACKET	= 6380,
		DEFAULT_GPS_SPEED				= 19200,
		DEFAULT_SAMPLING_FREQUENCY		= 1000,
		DEFAULT_LIMITS_TIME_BEFORE		= 20,
		DEFAULT_LIMITS_TIME_AFTER		= 10,
		DEFAULT_LIMITS_MIN_ACCELERATION	= 512-96,
		DEFAULT_LIMITS_MAX_ACCELERATION	= 512+96,
		DEFAULT_WRITING_INTERVAL		= 60
	};

	/** Acceleration limits to one sensor. */
	typedef struct {
		uint16_t	MinX;
		uint16_t	MaxX;
		uint16_t	MinY;
		uint16_t	MaxY;
		uint16_t	MinZ;
		uint16_t	MaxZ;
	} AccelerationMinMax;

	/** Sensors start reading offset, in CPU ticks */
	extern int					SensorsTicksOffset;
	/** Sensors byte length, in CPU ticks. */
	extern int					SensorsTicksByte;
	/** Sensors packet size, in CPU ticks. */
	extern int					SensorsTicksPacket;

	/** GPS baud rate. */
	extern unsigned int			GpsBaudRate;
	/** Acceleration sensors sampling frequency. */
	extern unsigned int			SamplingFrequency;

	/** Seconds of data to store before 'accident'. */
	extern uint16_t				LimitsTimeBefore;
	/** Seconds of data to store after 'accident'. */
	extern uint16_t				LimitsTimeAfter;
	/** Default acceleration limits to the sensors. */
	extern AccelerationMinMax	LimitsDefault;
	/** Acceleration limits to the sensors. */
	extern AccelerationMinMax	LimitsAcceleration[LoggerIO::SENSORS_MAX_PACKETS];

	/** Number of seconds between writing sessions. */
	extern unsigned int			WritingInterval;

	/** Load configuration file. */
	void
	Load(
		Filesystem::FAT16&	filesystem,
		const char*			filename
	);

	/** Print configuration file to debug output. */
	void
	PrintToDebug();
}; // namespace LoggerConfig

#endif /* LoggerConfig_h_ */

