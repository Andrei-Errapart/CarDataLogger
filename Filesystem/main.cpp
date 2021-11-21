/**
vim: ts=4
vim: shiftwidth=4
*/
#include <stdexcept>
#include <exception>
#include <vector>		// std::vector
#include <stdio.h>
#include <string.h>		// memcpy

#include <Filesystem_Config.h>
#include <Filesystem/Blockdevice_File.h>
#include <Filesystem/FAT16.h>
#include <Filesystem/File.h>

#include "LoggerIO.h"
#include "LoggerConfig.h"

using namespace Filesystem;

const char*	log_filename = "LOGGER.BIN";
const char*	ini_filename = "LOGGER.INI";


//*******************************************************************
static void
test_simple(
	const char*	disk_filename
)
{
	printf("FAT16 library test...\n");
	Blockdevice_File	disk(disk_filename);
	FAT16				filesys(disk);

	if (true) {
		LoggerConfig::Load(filesys, ini_filename);
		LoggerConfig::PrintToDebug();
	}

	if (false) {
		// Writing.
		for (unsigned int ntries = 0; ntries<2; ++ntries) {
			{
				File				f(filesys, log_filename, OPEN_CREATE);

				char				buffer[384];
				for (unsigned int i=0; i<7; ++i) {
					const char		c = '0' + (i % 10);
					memset(buffer, c, sizeof(buffer));
					f.Write(buffer, sizeof(buffer));
				}
			}

			{
				File				f(filesys, log_filename, OPEN_READONLY);

				printf("File '%s' size is %d bytes.\n", log_filename, f.Size());

				char	xbuf[1024+1] = { 0 };
				f.Read(xbuf, 1024);
				printf("File contents: %s\n", xbuf);
			}
		}
	}

	// Writing (again).
	if (false) {
		File				f(filesys, log_filename, OPEN_CREATE);

		f.SeekSet(f.Size());
		char				buffer[384];
		for (unsigned int i=0; i<7; ++i) {
			const char		c = '0' + (i % 10);
			memset(buffer, c, sizeof(buffer));
			f.Write(buffer, sizeof(buffer));
		}
	}

	// Reading.
	if (false) {
		File				f(filesys, log_filename, OPEN_READONLY);

		printf("File '%s' size is %d bytes.\n", log_filename, f.Size());

		char	xbuf[1024+1] = { 0 };
		f.Read(xbuf, 1024);
		printf("File contents: %s\n", xbuf);
	}
}

//*******************************************************************
template <class T>
void
append(
	std::vector<char>&	buffer,
	const T&			obj
)
{
	const char*	ptr = reinterpret_cast<const char*>(&obj);

	for (unsigned int i=0; i<sizeof(T); ++i) {
		buffer.push_back(ptr[i]);
	}
}

//*******************************************************************
#define	F_CPU	3

//*******************************************************************
static unsigned int
GetTSC()
{
	static unsigned int	tsc = 0;
	return ++tsc;
}

//*******************************************************************
static void
test_logging(
	const char*	disk_filename
)
{
	std::vector<char>		real_data;

	// 1. Write :)
	for (unsigned int round=0; round<3; ++round) {
		Blockdevice_File	disk(disk_filename);
		FAT16				filesys(disk);
		File				f(filesys, log_filename, OPEN_CREATE);
		const unsigned int	filesize = f.Size();

		// 1. seek to the end :)
		f.SeekSet(filesize);

		// Hello packet.
		{
			LoggerIO::HELLO			PacketHELLO;
			PacketHELLO.Magic 		= LoggerIO::MAGIC;
			PacketHELLO.Frequency	= F_CPU;
			PacketHELLO.Tick		= GetTSC();
			f.Write(&PacketHELLO, sizeof(PacketHELLO));
			append(real_data, PacketHELLO);
		}

		LoggerIO::SENSORS				PacketSENSORS;
		for (unsigned int i=0; i<10*1000; ++i) {
			memset(&PacketSENSORS, 0, sizeof(PacketSENSORS));
			PacketSENSORS.Header.Tick = GetTSC();
			PacketSENSORS.Header.Type = LoggerIO::TYPE_SENSORS;
			PacketSENSORS.Header.TotalSize = sizeof(PacketSENSORS);

			f.Write(&PacketSENSORS, sizeof(PacketSENSORS));
			append(real_data, PacketSENSORS);
		}
	}

	// 2. Verify with the real data.
	{
		Blockdevice_File	disk(disk_filename);
		FAT16				filesys(disk);
		File				f(filesys, log_filename, OPEN_READONLY);
		const unsigned int	filesize = f.Size();
		unsigned int		mismatch_count = 0;

		printf("File size: %d\n", filesize);

		std::vector<char>	new_data(filesize);
		f.Read(&new_data[0], new_data.size());

		for (unsigned int i=0; i<filesize; ++i) {
			if (real_data[i] != new_data[i]) {
				printf("BAD MISMATCH at %d\n", i);
				++mismatch_count;
			}
		}
		printf("Total %d mismatches.\n", mismatch_count);
	}
}

//*******************************************************************
static void
test_config(
	const char*	disk_filename
)
{
	Blockdevice_File	disk(disk_filename);
	FAT16				filesys(disk);

	LoggerConfig::Load(filesys, "Logger.ini");
	LoggerConfig::PrintToDebug();
}

//*******************************************************************
int
main(
	int	argc,
	char**	argv)
{
	const char*	disk_filename = argc>1 ? argv[1] : "test1_empty.fat";

	try {
		// test_logging(disk_filename);
		test_config(disk_filename);
	} catch (const std::exception& e) {
		printf("Exception: %s\n", e.what());
	}
	return 0;
}
