/**
vim: ts=4
vim: shiftwidth=4
*/
#include <string.h>				// memset, strncmp
#include <ctype.h>				// toupper.
#include <Filesystem/FAT16.h>
#include <Filesystem/Error.h>
#include <Filesystem/Endian.h>

#include <Filesystem_Config.h>

namespace Filesystem {

/** Special cluster numbers. */
typedef enum {
	/** Available. */
	CLUSTER_AVAILABLE		= 0x0000,
	/** Used for data. */
	CLUSTER_USED_MIN		= 0x0002,
	/** Used for data. */
	CLUSTER_USED_MAX		= 0xFFEF,
	/** Reserved, do not touch. */
	CLUSTER_RESERVED_MIN	= 0xFFF0,
	/** Reserved, do not touch. */
	CLUSTER_RESERVED_MAX	= 0xFFF6,
	/** Bad. */
	CLUSTER_BAD				= 0xFFF7,
	/** Last cluster in file. */
	CLUSTER_LAST_MIN		= 0xFFF8,
	/** Last cluster in file. */
	CLUSTER_LAST_MAX		= 0xFFFF
} CLUSTER;

/** File attributes, bit-or. */
typedef enum {
	ATTRIB_VOLUME		= 0x01,
	ATTRIB_DIRECTORY	= 0x02,
	ATTRIB_HIDDEN		= 0x04,
	ATTRIB_SYSTEM		= 0x08,
	ATTRIB_READONLY		= 0x10,
	ATTRIB_ARCHIVE		= 0x20,
	ATTRIB_NOT_FILE		= ATTRIB_VOLUME  | ATTRIB_DIRECTORY | ATTRIB_HIDDEN,
	ATTRIB_LFN			= ATTRIB_READONLY | ATTRIB_SYSTEM | ATTRIB_HIDDEN | ATTRIB_VOLUME
} ATTRIB;

//*******************************************************************
enum {
	DIRENTRY_FREE		= 0xE5,
	DIRENTRY_LAST		= 0x00
};

//*******************************************************************
#define	ARRAYSIZE(v)	(sizeof(v)/sizeof(v[0]))

//*******************************************************************
static inline uint16_t
uint16_of_byte2(
	const unsigned char*	ptr
)
{
	const uint16_t	r =  ((unsigned int)(ptr)[0]) +	
						(((unsigned int)(ptr)[1])<<8);
	return r;
}

//*******************************************************************
static inline uint32_t
uint32_of_byte4(
	const unsigned char*	ptr
)
{
	const uint32_t	r =  ((unsigned int)(ptr)[0]) +	
						(((unsigned int)(ptr)[1])<<8) +
						(((unsigned int)(ptr)[2])<<16) +
						(((unsigned int)(ptr)[3])<<24);
	return r;
}

//*******************************************************************
static bool
IsFilesystemFat16(
	Blockdevice&		Device,
	const unsigned int	PartitionStartBlock,
	unsigned char*		Buffer
)
{
	if (Device.Read(PartitionStartBlock, Buffer)) {
		return strncmp((const char*)(Buffer+54), "FAT16", 5)==0;
	}
	return false;
}

//*******************************************************************
FAT16::FAT16(
	Blockdevice&	device
)
:	Device_(device)
	,PartitionStartBlock_(0)
	,DataStartBlock_(-1)
	,RootDirBlock_(-1)
	,BlocksPerFat_(-1)
	,BlocksPerCluster_(-1)
	,FatBlock_(-1)
	,NrOfFATs_(-1)
	,NrOfBlocksInRootDir_(-1)
	,LastEmptyBlock_(-1)
	,FreeClusterSearchStart_(CLUSTER_USED_MIN)
	,DirectoryEntries_(device, FixFat16DirectoryEntryEndian)
	,FatEntries_(device,
#if defined(_MSC_VER)
				NULL
#else
				FixEndian16
#endif
	)
{
	unsigned char	first_block[Blockdevice::BLOCK_SIZE];

	// 1. Find partition start block.
	if (!IsFilesystemFat16(Device_, PartitionStartBlock_, first_block)) {
		const unsigned int	offset = uint32_of_byte4(&first_block[454]);
		filesystem_dprintf(("FAT16: checking partition start at block %d\n", offset));
		if (!IsFilesystemFat16(Device_, offset, first_block)) {
			throw Error("No FAT16 detected on the given block device.");
		}
		PartitionStartBlock_ = offset;
	}
	filesystem_dprintf(("FAT16: partition start block %d\n", PartitionStartBlock_));

	// 2. Load filesystem parameters.
	const unsigned int	maxRootEntry = uint16_of_byte2(first_block+17);

	BlocksPerCluster_	= first_block[13];
	FatBlock_			= uint16_of_byte2(first_block+14);
	NrOfFATs_			= first_block[16];
	BlocksPerFat_		= uint16_of_byte2(first_block+22);
	RootDirBlock_		= FatBlock_ + BlocksPerFat_ * NrOfFATs_;
	NrOfBlocksInRootDir_= ( maxRootEntry / Blockdevice::BLOCK_SIZE ) * 32;
	DataStartBlock_		= RootDirBlock_ + NrOfBlocksInRootDir_;

	DirectoryEntries_.SetOptions(
		PartitionStartBlock_ + RootDirBlock_,
		PartitionStartBlock_ + RootDirBlock_ + NrOfBlocksInRootDir_
	);

	FatEntries_.SetOptions(
		PartitionStartBlock_ + FatBlock_,
		PartitionStartBlock_ + FatBlock_ + BlocksPerFat_,
		BlocksPerFat_
	);

	filesystem_dprintf(("FAT16: BlocksPerCluster_=%d, maxRootEntry=%d, BlocksPerFat_=%ld\n",
			BlocksPerCluster_, FatBlock_, maxRootEntry, BlocksPerFat_));
	filesystem_dprintf(("FAT16: RootDirBLock_=0x%04X, FatBlock_=0x%04X, DataStartBlock_=0x%04X\n",
			PartitionStartBlock_+RootDirBlock_,
			PartitionStartBlock_+FatBlock_, 
			PartitionStartBlock_+DataStartBlock_));

	// 3. Flush open files list.
	for (unsigned int i=0; i<ARRAYSIZE(Files_); ++i) {
		memset(&Files_[i], 0, sizeof(Files_[i]));
	}
}

//*******************************************************************
unsigned int
FAT16::Open(
	const char*	filename,
	OPEN_FLAGS	flags
)
{
	filesystem_dprintf(("FAT16::Open '%s', flags=%d\n", filename, flags));

	// 1. Find free file descriptor.
	int		fd = -1;
	for (unsigned int i=0; i<ARRAYSIZE(Files_); ++i) {
		if (!Files_[i].IsOpen) {
			fd = i;
			break;
		}
	}
	if (fd<0) {
		throw Error("FAT16: not enough file descriptors for file '%s'", filename);
	}
	FatFile&	file = Files_[fd];

	// 2. Prepare filename buffer in the uppercase form 'FILENAMEEXT'.
	char	fatname[FILENAME_LENGTH+1];
	{
		memset(fatname, ' ', sizeof(fatname));
		unsigned int	dst_i = 0;
		for (unsigned int i=0; filename[i]!=0 && dst_i<FILENAME_LENGTH; ++i, ++dst_i) {
			if (filename[i]=='.') {
				dst_i = 7; // one character before dot.
			} else {
				fatname[dst_i] = toupper(filename[i]);
			}
		}
		// zero last character, too.
		fatname[FILENAME_LENGTH] = 0;
	}
	filesystem_dprintf(("FAT16: filename is '%s', entry size %d\n", fatname, sizeof(Fat16DirectoryEntry)));

	// 3. Scan the root directory.
	for (unsigned int block_nr=0; block_nr<NrOfBlocksInRootDir_; ++block_nr) {
		const Fat16DirectoryEntry*	fatpage = DirectoryEntries_.Fetch(block_nr);
		bool						stop_scan = false;
		for (unsigned int j=0; j<DirectoryEntries_.size(); ++j) {
			const Fat16DirectoryEntry&	entry = fatpage[j];
			if ((entry.Attributes & ATTRIB_NOT_FILE) != 0 || entry.Name[0]==DIRENTRY_FREE) {
				continue;
			}
			if (entry.Name[0] == DIRENTRY_LAST) {
				stop_scan = true;
				break;
			}
			if (strncmp(fatname, reinterpret_cast<const char*>(entry.Name), FILENAME_LENGTH)==0) {
				file.IsOpen					= true;
				file.Flags					= flags;
				file.FirstCluster			= entry.Cluster;
				file.Size					= entry.Size;
				file.SizeBlocks				= (entry.Size + BLOCK_SIZE-1) / BLOCK_SIZE;
				file.SizeClusters			= (file.SizeBlocks + BlocksPerCluster_ - 1) / BlocksPerCluster_;
				file.DirectoryBlock			= block_nr;
				file.DirectoryBlockIndex	= j;
				file.CurrentCluster			= entry.Cluster;
				file.RelativeBlock			= 0;
				file.RelativeCluster		= 0;
				filesystem_dprintf(("FAT16: file '%s' found, cluster=%d, size=%d, dir.block=%d\n",
					filename, file.FirstCluster, file.Size, file.DirectoryBlock));
				return fd;
			}
		}

		if (stop_scan) {
			break;
		}
	}

	if (flags == OPEN_CREATE) {
		// create the file entry :)
		for (unsigned int block_nr=0; block_nr<NrOfBlocksInRootDir_; ++block_nr) {
			const Fat16DirectoryEntry*	fatpage = DirectoryEntries_.Fetch(block_nr);
			for (unsigned int j=0; j<DirectoryEntries_.size(); ++j) {
				const Fat16DirectoryEntry&	entry = fatpage[j];
				if (entry.Name[0] == DIRENTRY_LAST || entry.Name[0] == DIRENTRY_FREE) {
					Fat16DirectoryEntry	new_entry;
					memset(&new_entry, 0, sizeof(new_entry));
					memcpy(new_entry.Name, fatname, FILENAME_LENGTH);

					// Set the date and time to January 02, 2005 21:00:00 .
					// If a real time clock is available, we could use it.
					new_entry.CreatedTime_HourAndMinute =  ((04 >> 1) & 0x1F + (03 << 5))
														 |(((03 >> 3) & 0x07) + (21 << 3)) << 8;
					new_entry.Time = new_entry.CreatedTime_HourAndMinute;
					new_entry.Date = (02 & 0x1F) | ((01 << 5) & 0xE0) |
									 ((01 >> 3) & 0x01 | (((2005 - 1980) & 0xFF) << 1) & 0xFE) << 8;
					new_entry.LastAccessedDate = new_entry.Date;

					DirectoryEntries_[j] = new_entry;

					file.IsOpen					= true;
					file.Flags					= flags;
					file.FirstCluster			= new_entry.Cluster;
					file.Size					= new_entry.Size;
					file.SizeBlocks				= 0;
					file.SizeClusters			= 0;
					file.DirectoryBlock			= block_nr;
					file.DirectoryBlockIndex	= j;
					file.CurrentCluster			= 0;
					file.RelativeBlock			= 0;
					file.RelativeCluster		= 0;
					filesystem_dprintf(("FAT16: file '%s' created, dir.block=%d\n",
						filename, file.DirectoryBlock));
					return fd;
				}
			}
		}
		throw Error("FAT16: unable to create file '%s'.", filename);
	} else {
		throw Error("FAT16: file '%s' not found in the root directory.", filename);
	}
}

//*******************************************************************
void
FAT16::Close(
	const unsigned int	fd
)
{
	filesystem_dprintf(("FAT16::Close\n"));

	// FIXME: flush operation!!!
	Files_[fd].IsOpen = false;

	DirectoryEntries_.Flush();
	FatEntries_.Flush();
}

//*******************************************************************
unsigned int
FAT16::Size(
	const unsigned int	fd
)
{
	return Files_[fd].Size;
}

//*******************************************************************
void
FAT16::Read(
	const unsigned int	fd,
	void*				block
)
{
	filesystem_dprintf(("FAT16::Read\n"));
	FatFile&			file = Files_[fd];

	if (file.RelativeBlock < file.SizeBlocks) {
		// 1. Read the file.
		const unsigned int	block_nr =   DataStartBlock_ +
										(file.CurrentCluster-2)*BlocksPerCluster_ +
										(file.RelativeBlock % BlocksPerCluster_);
		ReadDevice(block_nr, block);
	} else {
		throw Error("FAT16: end of file reached when reading.");
	}
}

//*******************************************************************
void
FAT16::Write(
	const unsigned int	fd,
	const void*			block
)
{
	filesystem_dprintf(("FAT16::Write\n"));

	FatFile&	file = Files_[fd];
	const bool	past_eof = file.RelativeBlock == file.SizeBlocks;

	// Do we need to find a new cluster?
	if (past_eof && (file.SizeBlocks % BlocksPerCluster_)==0) {
		const unsigned int	fatentries_per_block = (BLOCK_SIZE / sizeof(FatEntry));
		unsigned int		cluster = FreeClusterSearchStart_;

		for (unsigned int scan_count=0; scan_count<BlocksPerFat_; ++scan_count) {
			const unsigned int	block_nr = cluster / fatentries_per_block;
			const unsigned int	start_index = cluster % fatentries_per_block;
			bool				found = false;
			unsigned int		found_i = -1;
			FatEntry*			fatpage = FatEntries_.Fetch(block_nr);

			filesystem_dprintf(("FAT16::Write searches for free cluster in block %d, start index %d\n", block_nr, start_index));
			for (unsigned int i=start_index; i<fatentries_per_block; ++i) {
				if (fatpage[i] == CLUSTER_AVAILABLE) {
					found = true;
					found_i = i;
					break;
				}
			}

			if (found) {
				filesystem_dprintf(("FAT16::Write found free cluster %d\n", cluster));
				cluster = block_nr * fatentries_per_block + found_i;
				FreeClusterSearchStart_ = cluster + 1;

				// 1. Write data to the block.
				const unsigned int	data_block_nr = DataStartBlock_ + (cluster - 2)*BlocksPerCluster_;
				WriteDevice(data_block_nr, block);

				// 2. Set new cluster to be the last one.
				FatEntries_[found_i] = CLUSTER_LAST_MAX;

				// 4. Update file size and current cluster.
				const unsigned int	previous_cluster = file.CurrentCluster;
				file.SizeBlocks		= file.SizeBlocks + 1;
				file.SizeClusters	= file.SizeClusters + 1;
				file.Size			= file.SizeBlocks * BLOCK_SIZE;
				file.CurrentCluster = cluster;
				const unsigned int	new_size = file.SizeBlocks * BLOCK_SIZE;

				// Was it first write into empty file?
				if (file.SizeBlocks == 1) {
					// 3. Update directory entries, including fat.
					file.FirstCluster = cluster;

					Fat16DirectoryEntry	direntry = DirectoryEntries_.Fetch(file.DirectoryBlock)[file.DirectoryBlockIndex];
					direntry.Cluster = cluster;
					direntry.Size = new_size;
					DirectoryEntries_[file.DirectoryBlockIndex] = direntry;
				} else {
					// 3. Set the previous cluster to point to the new cluster.
					FatEntries_.Fetch(previous_cluster / fatentries_per_block);
					FatEntries_[previous_cluster % fatentries_per_block] = file.CurrentCluster;

					// 4. Update directory entries for new size.
					Fat16DirectoryEntry	direntry = DirectoryEntries_.Fetch(file.DirectoryBlock)[file.DirectoryBlockIndex];
					direntry.Size = new_size;
					DirectoryEntries_[file.DirectoryBlockIndex] = direntry;
				}

				// yes :)
				return;
			} else {
				cluster = ((cluster/fatentries_per_block + 1) % BlocksPerFat_) * fatentries_per_block;
			}
		}

		// Perhaps we failed.
		throw Error("FAT16: Disk Full.");
	}

	// Write :)
	const unsigned int	data_block_nr = DataStartBlock_ +
										(file.CurrentCluster - 2)*BlocksPerCluster_ +
										(file.RelativeBlock % BlocksPerCluster_);
	WriteDevice(data_block_nr, block);


	// Is file size increased by one block?
	if (past_eof) {
		file.SizeBlocks += 1;
		file.Size	= file.SizeBlocks * BLOCK_SIZE;
		const unsigned int	new_size = file.SizeBlocks * BLOCK_SIZE;
		Fat16DirectoryEntry	direntry = DirectoryEntries_.Fetch(file.DirectoryBlock)[file.DirectoryBlockIndex];
		direntry.Size = new_size;
		DirectoryEntries_[file.DirectoryBlockIndex] = direntry;
	} else {
		// shall we set new size because last block of the file was written to?
		const unsigned int	full_size = file.SizeBlocks * BLOCK_SIZE;
		if (file.RelativeBlock+1==file.SizeBlocks && full_size!=file.Size) {
			Fat16DirectoryEntry	direntry = DirectoryEntries_.Fetch(file.DirectoryBlock)[file.DirectoryBlockIndex];
			direntry.Size = full_size;
			DirectoryEntries_[file.DirectoryBlockIndex] = direntry;
		}
	}

	// printf("write not implemented.\n");
	// FIXME: do what?
}

//*******************************************************************
void
FAT16::SeekSetBlock(
	const unsigned int	fd,
	const unsigned int	BlockNr
)
{
	FatFile&			file = Files_[fd];

	if (BlockNr <= file.SizeBlocks) {
		const unsigned int	current_relative_cluster	= file.RelativeBlock / BlocksPerCluster_;
		const unsigned int	new_relative_cluster		= BlockNr / BlocksPerCluster_;

		if (current_relative_cluster != new_relative_cluster) {
			// Update file.CurrentCluster
			unsigned int	cluster	= -1;
			unsigned int	count	= -1;
			if (new_relative_cluster < current_relative_cluster) {
				cluster = file.FirstCluster;
				count	= new_relative_cluster;
			} else {
				cluster	= file.CurrentCluster;
				count	= new_relative_cluster - current_relative_cluster;
			}

			FatEntry*			fatpage = 0;
			const unsigned int	fatentries_per_block = (BLOCK_SIZE / sizeof(FatEntry));
			for (; count>0; --count) {
				if (cluster>=CLUSTER_USED_MIN && cluster<=CLUSTER_USED_MAX) {
					// yes, take on the next one.
					const unsigned int	block_nr = cluster / fatentries_per_block;
					const unsigned int	index = cluster % fatentries_per_block;
					fatpage = FatEntries_.Fetch(block_nr);
					if (count!=1 || BlockNr != file.SizeBlocks) {
						// yeah, skip the damn thing :)
						cluster = fatpage[index];
					}
				} else {
					throw Error("FAT16: next cluster %d invalid in FAT.", cluster);
				}
			}

			if (cluster<CLUSTER_USED_MIN || cluster >CLUSTER_USED_MAX) {
				throw Error("FAT16::Seek: cluster %d is out of range [%d .. %d].", cluster, CLUSTER_USED_MIN, CLUSTER_USED_MAX);
			}

			file.CurrentCluster = cluster;
			file.RelativeCluster = new_relative_cluster;
		}

		file.RelativeBlock = BlockNr;
	} else if (BlockNr == file.SizeBlocks) {
		// signal EOF.
		file.RelativeBlock = BlockNr;
		if (file.RelativeBlock % BlocksPerCluster_ == 0) {
			++file.RelativeCluster;
		}
	} else {
		throw Error("FAT16::Seek: seeking too far away from the end of file (block %d).", BlockNr);
	}
}

//*******************************************************************
void
FAT16::SetSize(
	const unsigned int	fd,
	const unsigned int	NewSize
)
{
	FatFile&			file = Files_[fd];

	// 1. Check end cluster.
	const unsigned int	new_size_clusters = (NewSize + BlocksPerCluster_*BLOCK_SIZE - 1) / (BlocksPerCluster_*BLOCK_SIZE);
	if (new_size_clusters != file.SizeClusters) {
		throw Error("FAT16::SetSize: new size ends in different cluster than the old size.");
	}

	// 2. Check current position.
	const unsigned int	new_size_blocks = (NewSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
	if (new_size_blocks < file.RelativeBlock) {
		throw Error("FAT16::SetSize: new size is less than current file pointer.");
	}

	// 2. Update :)
	file.Size = NewSize;
	file.SizeBlocks = (NewSize + BLOCK_SIZE - 1) / BLOCK_SIZE;

	Fat16DirectoryEntry	direntry = DirectoryEntries_.Fetch(file.DirectoryBlock)[file.DirectoryBlockIndex];
	// Shall we update?
	if (direntry.Size != NewSize) {
		direntry.Size = NewSize;
		DirectoryEntries_[file.DirectoryBlockIndex] = direntry;
	}
}

//*******************************************************************
void
FAT16::Flush(
	const unsigned int	fd
)
{
	FatEntries_.Flush();
	DirectoryEntries_.Flush();
}

//*******************************************************************
void
FAT16::ReadDevice(
	const unsigned int	Nr,
	void*				Block
)
{
	Device_.Read(Nr + PartitionStartBlock_, Block);
}

//*******************************************************************
void
FAT16::WriteDevice(
	const unsigned int	Nr,
	const void*			Block
)
{
	Device_.Write(Nr + PartitionStartBlock_, Block);
}

//*******************************************************************
void
FAT16::FixFat16DirectoryEntryEndian(
	Fat16DirectoryEntry&	entry
)
{
	FixEndian16(entry.CreatedTime_HourAndMinute);
	FixEndian16(entry.CreatedDate);
	FixEndian16(entry.LastAccessedDate);
	FixEndian16(entry.ExtendedAttribute);
	FixEndian16(entry.Time);
	FixEndian16(entry.Date);
	FixEndian16(entry.Cluster);
	FixEndian32(entry.Size);
}

} // namespace Filesystem
