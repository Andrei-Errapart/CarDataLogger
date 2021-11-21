/** Logger datafile convert tool. */
#include <stdexcept>
#include <exception>	// std::exception
#include <string>		// std::string

#include <stdio.h>		// fopen, etc.
#include "LoggerIO.h"

//*******************************************************************
enum {
	MAX_SENSORS	= 7,
	PACKET_SIZE = 4
};

//*******************************************************************
/** Acceleration limits to one sensor. */
typedef struct {
	unsigned int	MinX;
	unsigned int	MaxX;
	unsigned int	MinY;
	unsigned int	MaxY;
	unsigned int	MinZ;
	unsigned int	MaxZ;
} AccelerationMinMax;

//*******************************************************************
static void
DecodeData(
	const unsigned char*	buf,
	unsigned int*			x_axis, 
	unsigned int*			y_axis,
	unsigned int*			z_axis
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
	
	*x_axis = (unsigned int) decoded_data[0] | (unsigned int) decoded_data[1] << 8; 
	*y_axis = (unsigned int) decoded_data[2] | (unsigned int) decoded_data[3] << 8;
	*z_axis = (unsigned int) decoded_data[4] | (unsigned int) decoded_data[5] << 8;
}

//*******************************************************************
static void
convert_file(
	const std::string&	filename
)
{
	const std::string::size_type	dotpos = filename.rfind('.');
	const std::string				filename_prefix = dotpos==std::string::npos
										? filename
										: filename.substr(0, dotpos);

	FILE*	f = fopen(filename.c_str(), "rb");
	// FILE*	f = fopen("ahahahaha", "rb");
	if (f==0) {
		printf("File '%s' not found for reading.\n",
			filename.c_str());
		return;
	}
	fseek(f, 0, SEEK_END);
	const unsigned int		filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	printf("Opened file '%s', file size %d bytes\n",
		filename.c_str(), filesize);

	unsigned int					filecount = 0;

	for (;;) {
		// 1. Scan for the magic byte.
		LoggerIO::HELLO	hello = { 0, 0, 0 };
		{
			bool				found = false;
			volatile char*		magicbuffer = reinterpret_cast<char*>(&hello.Magic);
			volatile int*		magicptr = reinterpret_cast<int*>(&hello.Magic);
			const unsigned int	start_pos = ftell(f);
			do {
				const int	c = fgetc(f);
				if (c==EOF) {
					printf("File end reached.");
					break;
				}

				magicbuffer[0] = magicbuffer[1];
				magicbuffer[1] = magicbuffer[2];
				magicbuffer[2] = magicbuffer[3];
				magicbuffer[3] = c;

				found = *magicptr == LoggerIO::MAGIC;
			} while (!found);

			if (found) {
				// Yeah found :) Read rest of the hello packet.
				const unsigned int	headerpos = ftell(f) - sizeof(hello.Magic);
				printf("Hello at pos 0x%04x, skipping %d bytes.\n", headerpos, headerpos - start_pos);
				fseek(f, headerpos, SEEK_SET);
				fread(&hello, sizeof(hello), 1, f);
				printf("Frequency: %d\n", hello.Frequency);
				printf("Tick     : %d\n", hello.Tick);
			} else {
				printf("End of file reached.\n");
				break;
			}
		}

		// Open output file.
		std::string	filename_out;
		{
			filename_out.resize(filename_prefix.size() + 100);
			const int	n = sprintf(const_cast<char*>(filename_out.c_str()),
								"%s-%d.txt", filename_prefix.c_str(), ++filecount);
		}
		FILE*	fout = fopen(filename_out.c_str(), "w");
		if (fout == 0) {
			printf("File '%s' cannot be opened for writing.\n", filename_out.c_str());
			continue;
		}
		printf("Writing '%s'.", filename_out.c_str());
		fflush(stdout);

		// 2. Read rest of the packets until mismatch :)
		std::string		gps_line;
		unsigned int	last_ok_pos = ftell(f);
		unsigned int	sparse_output = 0;
		unsigned int	skipcount = 2;
		AccelerationMinMax	minmax[MAX_SENSORS];
		memset(minmax, 0, sizeof(minmax));
		for (;;) {
			// 1. Read header.
			LoggerIO::HEADER	header;
			{
				const int r1 = fread(&header, sizeof(header), 1, f);
				if (r1 != 1) {
					break;
				}
			}

			// 2. Read the rest.
			bool	packet_ok = false;
			switch (header.Type) {
			case LoggerIO::TYPE_GPS:
				if (header.TotalSize == sizeof(LoggerIO::GPS)) {
					LoggerIO::GPS	gps;
					gps.Header = header;
					const int	r = fread(gps.NmeaLine, sizeof(gps.NmeaLine), 1, f);
					if (r == 1) {
						gps_line = reinterpret_cast<const char*>(gps.NmeaLine);
						packet_ok = true;
					}
				} else {
					printf("GPS packet error.\n");
				}
				break;
			case LoggerIO::TYPE_SENSORS:
				if (header.TotalSize == sizeof(LoggerIO::SENSORS)) {
					LoggerIO::SENSORS	sensors;
					sensors.Header = header;
					const int	r = fread(sensors.Readings, sizeof(sensors.Readings), 1, f);
					if (r == 1) {
						if (skipcount>0) {
							--skipcount;
							continue;
						}
						fprintf(fout, "%d,", header.Tick);
						for (unsigned int i=0; i<MAX_SENSORS; ++i) {
							unsigned int	x;
							unsigned int	y;
							unsigned int	z;
							DecodeData(&sensors.Readings[i*PACKET_SIZE], &x, &y, &z);
							fprintf(fout, "%d,%d,%d,", x,y,z);
							
							// Update min, max.
							if (x!=0 && y!=0 && z!=0) {
								AccelerationMinMax&	mm = minmax[i];
								if (mm.MinX==0 || mm.MaxX==0 || mm.MinY==0 || mm.MaxY==0 || mm.MinZ==0 || mm.MaxZ==0) {
									mm.MinX = mm.MaxX = x;
									mm.MinY = mm.MaxY = y;
									mm.MinZ = mm.MaxZ = z;
								} else {
									if (x < mm.MinX) {
										mm.MinX = x;
									}
									if (x > mm.MaxX) {
										mm.MaxX = x;
									}
									if (y < mm.MinY) {
										mm.MinY = y;
									}
									if (y > mm.MaxY) {
										mm.MaxY = y;
									}
									if (z < mm.MinZ) {
										mm.MinZ = z;
									}
									if (z > mm.MaxZ) {
										mm.MaxZ = z;
									}
								}
							}
						}
						fprintf(fout, "%s\n", gps_line.c_str());
						if (++sparse_output > 60*1000) {
							sparse_output = 0;
							putchar('.');
							fflush(stdout);
						}
						packet_ok = true;
					}
				} else {
					printf("Acceleration sensors packet error.\n");
				}
				break;
			default:
				printf("Unknown packet!\n");
				break;
			}

			if (packet_ok) {
				last_ok_pos = ftell(f);
			} else {
				break;
			}
		}

		// print min-max
		printf("\nMinMax: ");
		for (unsigned int i=0; i<MAX_SENSORS; ++i) {
			AccelerationMinMax&	mm = minmax[i];
			printf("%d:[%d,%d %d,%d %d,%d] ", i, mm.MinX, mm.MaxX, mm.MinY, mm.MaxY, mm.MinZ, mm.MaxZ);
		}
		printf("\n");

		printf("End of file reached.\n");
		fclose(fout);

		fseek(f, last_ok_pos, SEEK_SET);
	}

	fclose(f);
}

//*******************************************************************
int
main(
	int	argc,
	char**	argv
)
{
	if (argc > 1) {
		for (int i=1; i<argc; ++i) {
			try {
				convert_file(argv[i]);
			} catch (const std::exception& e) {
				printf("Exception: %s\n", e.what());
			}
		}
	} else {
		printf("Usage:\n");
		printf("\tLogConvert input_file.bin [input_file2.bin] ... \n");
	}
	return 0;
}
