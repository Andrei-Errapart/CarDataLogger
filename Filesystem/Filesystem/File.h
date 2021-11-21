/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_File_h_
#define Filesystem_File_h_

#include <Filesystem/FAT16.h>

namespace Filesystem {

/** Conventional FILE* like interface to the block-based FAT16 system. */
class File {
	enum {
		BLOCK_SIZE = Blockdevice::BLOCK_SIZE
	};
public:
	File(
		FAT16&		filesys,
		const char*	filename,
		OPEN_FLAGS	flags
	);

	~File();

	void
	Read(
		void*				buffer,
		const unsigned int	size
	);

	/** Write N bytes to the file. */
	void
	Write(
		const void*			buffer,
		const unsigned int	size
	);

	/** Seek to the absolute position. */
	void
	SeekSet(
		const unsigned int	pos
	);

	/** Current file pointer position. */
	unsigned int
	Pos() const;

	/** Size of the file, in bytes. */
	unsigned int
	Size() const;

	/** Flush any pending writes. */
	void
	Flush();
private:
	char* Fetch(
		const unsigned int	block_nr
	);

	void
	FlushBuffer();
private:
	FAT16&				filesys_;
	const OPEN_FLAGS	flags_;
	unsigned int		fd_;
	unsigned int		size_blocks_;
	unsigned int		size_mod_blocks_;
	unsigned int		pos_blocks_;
	unsigned int		pos_mod_blocks_;

	unsigned int		buffer_block_nr_;
	char				buffer_[FAT16::BLOCK_SIZE];
	bool				buffer_valid_;
	bool				buffer_dirty_;
}; // class File

} // namespace File

#endif /* Filesystem_File_h_ */
