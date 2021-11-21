// vim: ts=4 shiftwidth=4
#ifndef CircularBuffer_h_
#define	CircularBuffer_h_

#include <stdbool.h>	// bool

/**
 * Lock-free circular buffer.
 */
template <class E>
class CircularBuffer {
public:
	/** Initialize buffer to empty state. */
	CircularBuffer(
		E*					buffer,
		const unsigned int	size
	)
	:	 buffer_(buffer)
		,size_(size)
		,push_index_(0)
		,pop_index_(0)
	{
	}

	/** Set buffer memory. */
	void
	SetBuffer(
		E*					buffer,
		const unsigned int	size
	)
	{
		buffer_ = buffer;
		size_ = size;
		push_index_ = push_index_ % size_;
		pop_index_ = pop_index_ % size_;

		Clear();
	}

	/** Is buffer empty? */
	bool
	IsEmpty() const
	{
		return push_index_ == pop_index_;
	}

	/** Is buffer full? */
	bool
	IsFull() const
	{
		return ((push_index_ + 1) % size_) == pop_index_;
	}

	/** Push element \c e into the buffer.
	 * \return true on success, false when buffer is full.
	 */
	bool
	Push(
		const E&	e
	)
	{
		const unsigned int next_push_index = (push_index_ + 1) % size_;
		if (next_push_index == pop_index_) {
			return false;
		} else {
			if (buffer_ != 0) {
				buffer_[push_index_] = e;
				push_index_ = next_push_index;
			}
			return true;
		}
	}

	/** Push an alread-present element, prepared by Poke(), into the buffer.
	 * \return true on success, false when buffer is full.
	 */
	bool
	Push()
	{
		const unsigned int next_push_index = (push_index_ + 1) % size_;
		if (next_push_index == pop_index_) {
			return false;
		} else {
			if (buffer_ != 0) {
				push_index_ = next_push_index;
			}
			return true;
		}
	}

	/** Pop an element, note that it doesn't check for failure!
	 * \return Popped element.
	 */
	E
	Pop()
	{
		const E	r = buffer_[pop_index_];
		pop_index_ = (pop_index_ + 1) % size_;
		return r;
	}

	/** Pop an element, check for failure.
	 * \return Popped element.
	 */
	bool
	Pop(
		E&	e
	)
	{
		if (pop_index_ == push_index_) {
			return false;
		} else {
			e = buffer_[pop_index_];
			pop_index_ = (pop_index_ + 1) % size_;
			return true;
		}
	}

	/** Clear the buffer. This is meant to be called by the consumer side. */
	void
	Clear()
	{
		pop_index_ = push_index_;
	}

	/** Approximate size of the buffer */
	unsigned int
	Size()
	{
		const unsigned int	ipop = pop_index_;
		const unsigned int	ipush = push_index_;
		return ipush>=ipop
			? ipush-ipop
			: ipush+size_-ipop;
	}

	/** Peek into the data buffer. No bounds checking.
	 * \param[in]	index	Index starting at current pop index.
	 */
	const E&
	Peek(
		const unsigned int	index
	) const
	{
		const unsigned int	ipop = (pop_index_ + index) % size_;
		return buffer_[ipop];
	}

	/** Get a pointer for poking the next to be pushed element in the buffer.
	 * Push() without arguments will push this element.
	 */
	E&
	Poke()
	{
		return buffer_[push_index_];
	}
private:
	/** Circular buffer. */
	E*						buffer_;
	/** Size of the buffer. */
	unsigned int			size_;
	/** Push index (heading). */
	volatile unsigned int	push_index_;
	/** Pop index (trailing). */
	volatile unsigned int	pop_index_;
}; // class CircularBuffer

#endif /* CircularBuffer_h_ */

