/**
vim: ts=4
vim: shiftwidth=4
*/
#include <stdlib.h>			// atoi.
#include <ctype.h>			// isdigit.

#include <Filesystem/Config.h>			// Very minimal configuration storage.
#include "LoggerConfig.h"	// ourselves.
#include "tprintf.h"		// tprintf


/** Global variables holding logger configuration. */
namespace LoggerConfig {
	enum {
		AMIN = DEFAULT_LIMITS_MIN_ACCELERATION,
		AMAX = DEFAULT_LIMITS_MAX_ACCELERATION
	};

	int					SensorsTicksOffset	= DEFAULT_SENSORS_TICKS_OFFSET;
	int					SensorsTicksByte	= DEFAULT_SENSORS_TICKS_BYTE;
	int					SensorsTicksPacket	= DEFAULT_SENSORS_TICKS_PACKET;
	unsigned int		GpsBaudRate			= DEFAULT_GPS_SPEED;
	unsigned int		SamplingFrequency	= DEFAULT_SAMPLING_FREQUENCY;
	uint16_t			LimitsTimeBefore	= DEFAULT_LIMITS_TIME_BEFORE;
	uint16_t			LimitsTimeAfter		= DEFAULT_LIMITS_TIME_AFTER;
	AccelerationMinMax	LimitsDefault		= { AMIN, AMAX, AMIN, AMAX, AMIN, AMAX } ;
	AccelerationMinMax	LimitsAcceleration[LoggerIO::SENSORS_MAX_PACKETS] = {
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX },
		{ AMIN, AMAX, AMIN, AMAX, AMIN, AMAX }
	};

	unsigned int			WritingInterval = DEFAULT_WRITING_INTERVAL;

	//*******************************************************************
	static bool
	atoi_array(
		Filesystem::Config&	cfg,
		const char*			section,
		const char*			key,
		uint16_t*			buffer,
		const unsigned int	buffer_size
		)
	{
		char*	ptr = cfg.Value(section, key);
		if (ptr == 0) {
			return false;
		}
		unsigned int	index = 0;

		while (*ptr!=0 && index<buffer_size) {
			// 1. search for isdigit
			while (*ptr!=0 && !isdigit(*ptr)) {
				++ptr;
			}
			if (*ptr == 0) {
				break;
			}

			// 2. convert :)
			buffer[index] = atoi(ptr);
			++index;

			// 3. search for non-isdigit.
			while (*ptr!=0 && isdigit(*ptr)) {
				++ptr;
			}
		}

		return index>=buffer_size;
	}

	//*******************************************************************
	static char	configbuffer[1024];
	void
	Load(
		Filesystem::FAT16&	filesystem,
		const char*			filename
	)
	{
		static const char*	section = "Logger";
		Filesystem::Config	cfg(filesystem, filename, configbuffer, sizeof(configbuffer));

		SensorsTicksOffset	= cfg.ValueAsInt(section, "SensorsTicksOffset",	DEFAULT_SENSORS_TICKS_OFFSET);
		SensorsTicksByte	= cfg.ValueAsInt(section, "SensorsTicksByte",	DEFAULT_SENSORS_TICKS_BYTE);
		SensorsTicksPacket	= cfg.ValueAsInt(section, "SensorsTicksPacket",	DEFAULT_SENSORS_TICKS_PACKET);
		GpsBaudRate			= cfg.ValueAsInt(section, "GPS",				DEFAULT_GPS_SPEED);
		SamplingFrequency	= cfg.ValueAsInt(section, "SamplingFrequency",	DEFAULT_SAMPLING_FREQUENCY);
		WritingInterval		= cfg.ValueAsInt(section, "WritingInterval",	DEFAULT_WRITING_INTERVAL);

		if (!atoi_array(cfg, section, "Limits_Default", &LimitsDefault.MinX, 6)) {
			LimitsDefault.MinX = AMIN;
			LimitsDefault.MaxX = AMAX;
			LimitsDefault.MinY = AMIN;
			LimitsDefault.MaxY = AMAX;
			LimitsDefault.MinZ = AMIN;
			LimitsDefault.MaxZ = AMAX;
		}

		for (unsigned int i=0; i<LoggerIO::SENSORS_MAX_PACKETS; ++i) {
			char	key[40];
			sprintf(key, "Limits_%d", i+1);
			if (!atoi_array(cfg, section, key, &LimitsAcceleration[i].MinX, 6)) {
				LimitsAcceleration[i] = LimitsDefault;
			}
		}

		uint16_t	buffer[2];
		if (atoi_array(cfg, section, "Limits_Time", buffer, 2)) {
			LimitsTimeBefore= buffer[0];
			LimitsTimeAfter	= buffer[1];
		} else {
			LimitsTimeBefore= DEFAULT_LIMITS_TIME_BEFORE;
			LimitsTimeAfter	= DEFAULT_LIMITS_TIME_AFTER;
		}
	}

	//*******************************************************************
	void
	PrintToDebug()
	{
		tprintf("[Logger]\n");
		tprintf("SensorsTicksOffset=%d\n",	SensorsTicksOffset);
		tprintf("SensorsTicksByte=%d\n",	SensorsTicksByte);
		tprintf("SensorsTicksPacket=%d\n",	SensorsTicksPacket);
		tprintf("GPS=%d\n", GpsBaudRate);
		tprintf("SamplingFrequency=%d\n", SamplingFrequency);
		tprintf("Limits_Default=%d %d %d %d %d %d\n", LimitsDefault.MinX, LimitsDefault.MaxX, LimitsDefault.MinY, LimitsDefault.MaxY, LimitsDefault.MinZ, LimitsDefault.MaxZ);
		for (unsigned int i=0; i<LoggerIO::SENSORS_MAX_PACKETS; ++i) {
			const AccelerationMinMax&	limits = LimitsAcceleration[i];
			tprintf("Limits_%d=%d %d %d %d %d %d\n", i+1, limits.MinX, limits.MaxX, limits.MinY, limits.MaxY, limits.MinZ, limits.MaxZ);
		}
		tprintf("Limits_Time=%d %d\n", LimitsTimeBefore, LimitsTimeAfter);
		tprintf("WritingInterval=%d\n", WritingInterval);
	}

}; // namespace LoggerConfig

