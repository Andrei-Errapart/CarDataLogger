/**
vim: ts=4
vim: shiftwidth=4
*/
#include <Filesystem/File.h>
#include <Filesystem/Error.h>

#include <string.h>	// memcpy

namespace Filesystem {

//*******************************************************************
File::File(
	FAT16&		filesys,
	const char*	filename,
	OPEN_FLAGS	flags
)
:
	filesys_(filesys)
	,flags_(flags)
	,pos_blocks_(0)
	,pos_mod_blocks_(0)
	,buffer_block_nr_(0)
	,buffer_valid_(false)
	,buffer_dirty_(false)
{
	fd_ = filesys_.Open(filename, flags);
	const unsigned int	size = filesys_.Size(fd_);
	size_blocks_ = size / BLOCK_SIZE;
	size_mod_blocks_ = size % BLOCK_SIZE;
}

//*******************************************************************
File::~File()
{
	try {
		Flush();
		filesys_.Close(fd_);
	} catch (const std::exception& e) {
		filesystem_dprintf(("File::~File Exception: '%s'\n", e.what()));
	}
}

//*******************************************************************
void
File::Read(
	void*				buffer,
	const unsigned int	size
)
{
	// Can we really read?
	if (Size() < Pos()+size) {
		throw Error("File: cannot read past the end of file.");
	}

	unsigned int	todo = size;
	char*			ptr = reinterpret_cast<char*>(buffer);

	while (todo>0) {
		// Fetch current block.
		const unsigned int	this_round =
			(pos_mod_blocks_ + todo) > BLOCK_SIZE
				? BLOCK_SIZE - pos_mod_blocks_
				: todo;
		const char*			block = Fetch(pos_blocks_);

		// Copy data.
		for (unsigned int i=0; i<this_round; ++i) {
			*ptr = block[pos_mod_blocks_ + i];
			++ptr;
		}

		// Adjust pointers.
		SeekSet(Pos() + this_round);
		todo -= this_round;
	}
}

//*******************************************************************
void
File::Write(
	const void*			buffer,
	const unsigned int	size
)
{
	// filesystem_dprintf(("File::Write %d bytes\n", size));

	if (flags_ == OPEN_READONLY) {
		throw Error("File: unable to write to file that is opened read-only.");
	}
	const char*		ptr = reinterpret_cast<const char*>(buffer);
	unsigned int	todo = size;
	char*			block = 0;

	while (todo>0) {
		// Fetch current block.
		const unsigned int	this_round =
			(pos_mod_blocks_ + todo) > BLOCK_SIZE
				? BLOCK_SIZE - pos_mod_blocks_
				: todo;

		// special case: exactly at the block past the end of file.
		if (pos_mod_blocks_==0 && size_mod_blocks_==0 && size_blocks_==pos_blocks_) {
			FlushBuffer();
			buffer_valid_ = false;
			buffer_block_nr_ = pos_blocks_;
			memset(buffer_, 0, sizeof(buffer_));
			block = buffer_;
		} else {
			block = Fetch(pos_blocks_);
		}

		// Update data.
		buffer_dirty_ = true;
		memcpy(block + pos_mod_blocks_, ptr, this_round);
		buffer_valid_ = true;

		// Adjust pointers.
		SeekSet(Pos() + this_round);
		// yeah, wrote past end :)
		if (pos_blocks_*BLOCK_SIZE+pos_mod_blocks_ > size_blocks_*BLOCK_SIZE+size_mod_blocks_) {
			size_blocks_		= pos_blocks_;
			size_mod_blocks_	= pos_mod_blocks_;
		}
		todo -= this_round;
		ptr += this_round;
	}
}

//*******************************************************************
void
File::SeekSet(
	const unsigned int	pos
)
{
	pos_blocks_		= pos / BLOCK_SIZE;
	pos_mod_blocks_	= pos % BLOCK_SIZE;
}

//*******************************************************************
unsigned int
File::Pos() const
{
	return pos_blocks_ * BLOCK_SIZE + pos_mod_blocks_;
}

//*******************************************************************
unsigned int
File::Size() const
{
	return size_blocks_ * BLOCK_SIZE + size_mod_blocks_;
}

//*******************************************************************
void
File::Flush()
{
	FlushBuffer();
	filesys_.SetSize(fd_, size_blocks_*BLOCK_SIZE + size_mod_blocks_);
	filesys_.Flush(fd_);
}

//*******************************************************************
char*
File::Fetch(
	const unsigned int	block_nr
)
{
	if (block_nr != buffer_block_nr_) {
		FlushBuffer();
	}

	if (!buffer_valid_ || block_nr != buffer_block_nr_) {
		buffer_valid_ = false;

		filesys_.SeekSetBlock(fd_, block_nr);
		filesys_.Read(fd_, buffer_);

		buffer_block_nr_ = block_nr;
		buffer_dirty_ = false;
		buffer_valid_ = true;
	}

	return buffer_;
}

//*******************************************************************
void
File::FlushBuffer()
{
	if (buffer_valid_ && buffer_dirty_) {
		filesys_.SeekSetBlock(fd_, buffer_block_nr_);
		filesys_.Write(fd_, buffer_);
		buffer_dirty_ = false;
	}
}

} // namespace Filesystem
