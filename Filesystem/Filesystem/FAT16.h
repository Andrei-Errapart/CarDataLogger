/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_FAT16_h_
#define	Filesystem_FAT16_h_

#include <Filesystem/BlockDevice.h>
#include <Filesystem/Blockbuffer.h>

#include <functional>			// pointer_to_unary_function :)
#include <stdbool.h>			// bool, true, false.
#include <stdint.h>				// int32, etc.

namespace Filesystem {

typedef enum {
	/** Open existing file for read/write. */
	OPEN_EXISTING,
	/** Open file, create if necessary. */
	OPEN_CREATE,
	/** Open file read-only, don't create. */
	OPEN_READONLY
} OPEN_FLAGS;



/** FAT16 filesystem running on top of some block device, probably SD/MMC memory card. */
class FAT16 {
public:
	enum {
		BLOCK_SIZE = Blockdevice::BLOCK_SIZE
	};
public:
	FAT16(
		Blockdevice&	device
	);

	/* BLOCK INTERFACE. */

	/** Open file for read/write.
	Throws exception on error.

	Block pointer will be set to the beginning of the file.

	TODO: scan directories other than the root, too.
	*/
	unsigned int
	Open(
		const char*	filename,
		OPEN_FLAGS	flags
	);

	/** Close file opened previously and flush the buffers. */
	void
	Close(
		const unsigned int	fd
	);

	/** Query file size. */
	unsigned int
	Size(
		const unsigned int	fd
	);

	/** Read one block from the current block pointer */
	void
	Read(
		const unsigned int	fd,
		void*				block
	);

	/** Write one block at the current block pointer.
	
	File size is changed when:
	1. Write occurs in the last block of the file and file size is not full multiple of block size.
	   New file size is set to the full multiple of blocks.
	2. Write occurs at the block after the last block in the file.
	*/
	void
	Write(
		const unsigned int	fd,
		const void*			block
	);

	/** Seek to the given block. Permits seeking one block past
	the file size -- writing to this position increments file size.
	*/
	void
	SeekSetBlock(
		const unsigned int	fd,
		const unsigned int	BlockNr
	);

	/** Set file size to new value.
	Limits:
	1. Must be within the last cluster of the current file size.
	2. Must be past the current position.
	*/
	void
	SetSize(
		const unsigned int	fd,
		const unsigned int	Size
	);

	/** Flush file and filesystem buffers. */
	void
	Flush(
		const unsigned int	fd
	);
private:
	void
	ReadDevice(
		const unsigned int	Nr,
		void*				Block
	);

	void
	WriteDevice(
		const unsigned int	Nr,
		const void*			Block
	);
private:
	typedef uint16_t		FatEntry;

	/** Internal representation of an opened file. */
	typedef struct
	{
		/** Is given file open? */
		bool				IsOpen;
		/** Open flags -- read only, etc. */
		OPEN_FLAGS			Flags;
		/** First file cluster.*/
		unsigned int		FirstCluster;
		/** File size, bytes. */
		unsigned int		Size;
		/** File size, blocks. */
		unsigned int		SizeBlocks;
		/** File size, clusters. */
		unsigned int		SizeClusters;
		/** Directory block of the file entry. */
		unsigned int		DirectoryBlock;
		/** Directory block index of the file entry. */
		unsigned int		DirectoryBlockIndex;
		/** Current cluster. If seeked past EOF (i.e. SizeBlocks==RelativeBlock
		and SizeClusters==RelativeClusters),
		it points to the previous cluster. */
		unsigned int		CurrentCluster;
		/** Block relative to the beginning of the file.
		If this equals to the SizeBlocks, it indicates EOF. */
		unsigned int		RelativeBlock;
		/** Cluster relative to the beginning of the file. */
		unsigned int		RelativeCluster;
	} FatFile;

	/** Number of characters in Fat16 filename length. */
	enum {
		FILENAME_LENGTH	 = 11
	};

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif
	typedef struct {
		/** Name of the file. */
		uint8_t		Name[FILENAME_LENGTH];			// 0..10
		/** Attributes. */
		uint8_t		Attributes;						// 11
		/** NT (Reserved for WindowsNT; always 0). */
		uint8_t		NT;								// 12
		/** Created time; millisecond portion. */
		uint8_t		CreatedTime_Millisecond;		// 13
		/** Created time; hour and minute. */
		uint16_t	CreatedTime_HourAndMinute;		// 14,15
		/** Created date. */
		uint16_t	CreatedDate;					// 16,17
		/** Last accessed date. */
		uint16_t	LastAccessedDate;				// 18,19
		/**   Extended Attribute (reserved for OS/2; always 0). */
		uint16_t	ExtendedAttribute;				// 20,21
		uint16_t	Time;							// 22,23
		uint16_t	Date;							// 24,25
		uint16_t	Cluster;						// 26,27
		uint32_t	Size;							// 28-31
	}
#if defined(__GNUC_)
	__attribute__((aligned(1)))
#endif
	Fat16DirectoryEntry;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

	static void
	FixFat16DirectoryEntryEndian(
		Fat16DirectoryEntry&	entry
	);

	/** Underlying block device. */
	Blockdevice&	Device_;
	/** List of open files. */
	FatFile			Files_[1];	// at the moment limited to one.

	/** Partition start block. */
	unsigned int	PartitionStartBlock_;
	/** Data start. */
	unsigned int	DataStartBlock_;
	/** Root directory. */
	unsigned int	RootDirBlock_;
	/** Blocks per FAT. */
	unsigned int	BlocksPerFat_;
	/** Blocks per cluster. */
	unsigned int	BlocksPerCluster_;
	/** FAT start block, this coincides number of reserved clusters. */
	unsigned int	FatBlock_;
	/** Number of FAT-s. */
	unsigned int	NrOfFATs_;
	/** Number of blocks in the root directory. */
	unsigned int	NrOfBlocksInRootDir_;
	/** Last searched empty block. */
	unsigned int	LastEmptyBlock_;
	/** Free cluster search start cluster. */
	unsigned int	FreeClusterSearchStart_;

	/** Directory entries, one block cached. */
	Blockbuffer<Fat16DirectoryEntry>	DirectoryEntries_;
	/** FAT entries, one block cached. */
	Blockbuffer<FatEntry>				FatEntries_;

}; // class FAT16

} // namespace Filesystem

#endif /* Filesystem_FAT16_h_ */
