#ifndef QUEUE_HH
#define	QUEUE_HH

#include "utilities.hh"
#include "time.hh"

//TODO imprvoe code size by creating a non-template base class with all common stuff
//TODO need to add some "peeking" API or iterator API, or some kind of deep-copy to another queue?
template<typename T>
class Queue
{
public:
	template<uint8_t SIZE>
	Queue(T (&buffer)[SIZE]): _buffer{buffer}, _mask{(uint8_t)(SIZE - 1)}, _head{0}, _tail{0}
	{
		static_assert(SIZE && !(SIZE & (SIZE - 1)), "SIZE must be a power of 2");
	}
	
	bool push(const T& item);
	bool pull(T& item);
	
	bool peek(T& item) const;
	uint8_t peek(T* buffer, uint8_t size) const;
	template<uint8_t SIZE>
	uint8_t peek(T (&buffer)[SIZE]) const;
	
	uint8_t items() const __attribute__((always_inline))
	{
		ClearInterrupt clint;
		return (_tail - _head) & _mask;
	}
	uint8_t free() const __attribute__((always_inline))
	{
		ClearInterrupt clint;
		return (_head - _tail - 1) & _mask;
	}
	
private:
	T* const _buffer;
	const uint8_t _mask;
	volatile uint8_t _head;
	volatile uint8_t _tail;
};

template<typename T>
bool Queue<T>::peek(T& item) const
{
	ClearInterrupt clint;
	if (_tail == _head) return false;
	item = _buffer[_head];
	return true;
}

template<typename T>
uint8_t Queue<T>::peek(T* buffer, uint8_t size) const
{
	ClearInterrupt clint;
	uint8_t actual = (_tail - _head) & _mask;
	if (size > actual) size = actual;
	//TODO optimize copy (index calculation is simple if split in 2 parts)
	for (uint8_t i = 0; i < size; ++i)
		buffer[i] = _buffer[(_head + i) & _mask];
	return size;
}

template<typename T>
template<uint8_t SIZE>
uint8_t Queue<T>::peek(T (&buffer)[SIZE]) const
{
	return peek(&buffer, SIZE);
}

template<typename T>
bool Queue<T>::push(const T& item)
{
	ClearInterrupt clint;
	if ((_head - _tail - 1) & _mask)
	{
		_buffer[_tail] = item;
		_tail = (_tail + 1) & _mask;
		return true;
	}
	return false;
}

template<typename T>
bool Queue<T>::pull(T& item)
{
	ClearInterrupt clint;
	if (_tail != _head)
	{
		item = _buffer[_head];
		_head = (_head + 1) & _mask;
		return true;
	}
	return false;
}

// Utility method that waits until a Queue has an item available
template<typename T>
T pull(Queue<T>& queue)
{
	T item;
	while (!queue.pull(item))
		Time::yield();
	return item;
}

#endif	/* QUEUE_HH */