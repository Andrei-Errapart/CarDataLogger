/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Blockbuffer_h_
#define Filesystem_Blockbuffer_h_

#include <stdexcept>
#include <exception>
#include <stdbool.h>

#include <Filesystem_Config.h>

#include <Filesystem/Blockdevice.h>
#include <Filesystem/Error.h>

namespace Filesystem {

/** One block buffer of objects on the disk. */
template <class Object>
class Blockbuffer {
public:
	typedef void (*FixObjectEndian)(Object& obj);

	enum {
		OBJECTS_SIZE = Blockdevice::BLOCK_SIZE / sizeof(Object)
	};

	//*******************************************************************
	Blockbuffer(
		Blockdevice&	Device,
		FixObjectEndian	FixEndian
	)
	:	Device_(Device)
		,FixObjectEndian_(FixEndian)
		,SecondCopyOffset_(-1)
		,RangeStart_(0)
		,RangeEnd_(-1)
		,BlockNr_(-1)
		,Valid_(false)
		,Dirty_(false)
	{
	}

	//*******************************************************************
	~Blockbuffer(
	)
	{
		try {
			Flush();
		} catch (const std::exception& e) {
			filesystem_dprintf(("Blockbuffer: error flushing, '%s'\n", e.what()));
		}
	}

	//*******************************************************************
	/** Set the range and second copy offset (if any).
	*/
	void
	SetOptions(
		const unsigned int	RangeStart,
		const unsigned int	RangeEnd,
		const int			SecondCopyOffset = -1
	)
	{
		RangeStart_ = RangeStart;
		RangeEnd_ = RangeEnd;
		SecondCopyOffset_ = SecondCopyOffset;
	}

	//*******************************************************************
	/** Fetch current object block. Block number is relative to the RangeStart. */
	Object*
	Fetch(
		const unsigned int	BlockNr
	)
	{
		if (BlockNr > (RangeEnd_-RangeStart_)) {
			throw Error("Blockbuffer: block %d is out of range [0 .. %d)", BlockNr, (RangeEnd_-RangeStart_));
		}

		if (BlockNr_ != BlockNr) {
			Flush();
		}

		if (!Valid_ || BlockNr_ != BlockNr) {
			Valid_ = false;

			Device_.Read(BlockNr+RangeStart_, Buffer_);
			FixBufferEndian_();

			BlockNr_ = BlockNr;
			Valid_ = true;
			Dirty_ = false;
		}

		return Buffer_;
	}

	//*******************************************************************
	void
	Flush()
	{
		if (Dirty_) {
			FixBufferEndian_();
			Device_.Write(BlockNr_+RangeStart_, Buffer_);
			if (SecondCopyOffset_>0) {
				Device_.Write(BlockNr_+RangeStart_ + SecondCopyOffset_, Buffer_);
			}
			FixBufferEndian_();
			Dirty_ = false;
		}
	}

	//*******************************************************************
	unsigned int
	size() const
	{
		return OBJECTS_SIZE;
	}

	//*******************************************************************
	inline Object&
	operator[](const unsigned int i)
	{
		Dirty_ = true;
		return Buffer_[i];
	}

	//*******************************************************************
	inline const Object&
	operator[](const unsigned int i) const
	{
		return Buffer_[i];
	}

private:
	//*******************************************************************
	void
	FixBufferEndian_()
	{
		if (FixObjectEndian_ != 0) {
			for (unsigned int i=0; i<OBJECTS_SIZE; ++i) {
				FixObjectEndian_(Buffer_[i]);
			}
		}
	}

	/** Buffered object page. */
	Object			Buffer_[OBJECTS_SIZE];

	/** Block device. */
	Blockdevice&	Device_;
	/** Function to fix/reverse fix object endianness. */
	FixObjectEndian	FixObjectEndian_;
	/** Offset to the second copy. -1 if not in use. */
	int				SecondCopyOffset_;
	/** Start range. */
	unsigned int	RangeStart_;
	/** End range. */
	unsigned int	RangeEnd_;

	/** Current block number. */
	unsigned int	BlockNr_;
	/** Is object block valid? */
	bool			Valid_;
	/** Is object block not yet written to disk? */
	bool			Dirty_;
}; // class Blockbuffer

} // namespace Filesystem


#endif /* Filesystem_Blockbuffer_h_ */
