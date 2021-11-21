// vim: ts=4 shiftwidth=4
#ifndef LoggerIO_h_
#define LoggerIO_h_

/** \file Logger data file structures.
 * Data shall be stored in the Little-Endian byte order.
 */

#include <stdint.h>			// 

#if defined(_MSC_VER)
#	pragma pack(push, 1)
#endif

#if defined(__GNUC_)
#	define	STRUCT_ALIGN_1	__attribute__((aligned(1)))
#else
#	define	STRUCT_ALIGN_1
#endif
namespace LoggerIO {

enum {
	MAGIC				= 0xF4D0BD07,
	SENSORS_PACKET_SIZE	= 4,
	SENSORS_MAX_PACKETS	= 7,
	SENSORS_BUFFER_SIZE = SENSORS_MAX_PACKETS * SENSORS_PACKET_SIZE
};

typedef enum {
	TYPE_HELLO		= 0x01,
	TYPE_GPS		= 0x02,
	TYPE_SENSORS	= 0x03
} TYPE;

/** Packet header. */
typedef struct {
	/** Type of the packet. */
	uint16_t	Type;
	/** Total size of the packet. */
	uint16_t	TotalSize;
	/** Ticks. */
	uint32_t	Tick;
} STRUCT_ALIGN_1 HEADER;

/** Hello. No payload */
typedef struct {
	/** Magic header. */
	uint32_t	Magic;
	/** Tick counter frequency. */
	uint32_t	Frequency;
	/** Starting tick. */
	uint32_t	Tick;
} HELLO;

/** Nmea data line from GPS. */
typedef struct {
	HEADER		Header;
	uint8_t		NmeaLine[100];
} STRUCT_ALIGN_1 GPS;

/** Acceleration sensor readings. */
typedef struct {
	HEADER		Header;
	uint8_t		Readings[SENSORS_BUFFER_SIZE];
} STRUCT_ALIGN_1 SENSORS;

}; // namespace LoggerIO

#if defined(_MSC_VER)
#	pragma pack(pop)
#endif

#if defined(__GNUC_)
#	undef	STRUCT_ALIGN_1
#endif

#endif /* LoggerIO_h_ */

