// Template implementation file intentionally included by MessageLib.hpp.

#ifndef MESSAGE_LIB_IMPL
#include "MessageLib.hpp"
#else

#include <chrono>

namespace Msg {

template<typename T>
CircularBuffer<T>::CircularBuffer(size_t capacity)
	: buffer_(capacity),
	  head_(0),
	  tail_(0),
	  size_(0),
	  capacity_(capacity) {
	if (capacity == 0) {
		throw std::invalid_argument("Capacity cannot be zero.");
	}
}

template<typename T>
void CircularBuffer<T>::killProcess() {
	std::lock_guard<std::mutex> lock(bufferMutex);
	// Notify all waiting threads to unblock (if any) before flushing the buffer
	m_BufferCv.notify_all();
	flush();
}

template<typename T>
void CircularBuffer<T>::push(const T& item) {
	std::lock_guard<std::mutex> lock(bufferMutex);
	// If empty, we need to notify any waiting threads that an item is being added
	bool isEmptyBeforePush = isEmpty();
	buffer_[head_] = item;
	head_ = (head_ + 1) % capacity_;
	if (size_ < capacity_) {
		size_++;
	} else {
		// If full, tail also moves forward
		tail_ = (tail_ + 1) % capacity_;
	}

	// Now we notify the waiting threads that an item has been added
	if (isEmptyBeforePush) {
		m_BufferCv.notify_all();
	}
}

template<typename T>
void CircularBuffer<T>::pop() {
	std::lock_guard<std::mutex> lock(bufferMutex);
	if (isEmpty()) {
		throw std::out_of_range("Buffer is empty.");
	}
	tail_ = (tail_ + 1) % capacity_;
	size_--;
}

template<typename T>
void CircularBuffer<T>::flush() {
	std::lock_guard<std::mutex> lock(bufferMutex);
	head_ = 0;
	tail_ = 0;
	size_ = 0;
}

template<typename T>
T& CircularBuffer<T>::operator[](size_t index) {
	if (index >= size_) {
		throw std::out_of_range("Index out of bounds.");
	}
	return buffer_[(tail_ + index) % capacity_];
}

template<typename T>
const T& CircularBuffer<T>::operator[](size_t index) const {
	if (index >= size_) {
		throw std::out_of_range("Index out of bounds.");
	}
	return buffer_[(tail_ + index) % capacity_];
}

template<typename T>
T& CircularBuffer<T>::peek(size_t index) {
	std::lock_guard<std::mutex> lock(bufferMutex);
	if (index >= size_) {
		throw std::out_of_range("Index out of bounds.");
	}
	return buffer_[(tail_ + index) % capacity_];
}

template<typename T>
T& CircularBuffer<T>::getHead(int timeout) {
	std::unique_lock<std::mutex> lock(bufferMutex);

	// If the buffer is empty, we wait for the signal that an item has been added
	if (isEmpty()) {
		if (timeout < 0) {
			m_BufferCv.wait(lock, [this] { return !isEmpty(); });
		} else {
			if (!m_BufferCv.wait_for(lock, std::chrono::milliseconds(timeout), [this] { return !isEmpty(); })) {
				// throw std::runtime_error("Timeout waiting for buffer item.");
			}
		}
	}

	size_t idx = (head_ + capacity_ - 1) % capacity_;
	return buffer_[idx];
}

template<typename T>
const T& CircularBuffer<T>::getHead() const {
	std::unique_lock<std::mutex> lock(bufferMutex);

	// If the buffer is empty, we wait for the signal that an item has been added
	if (isEmpty()) {
		m_BufferCv.wait(lock, [this] { return !isEmpty(); });
	}

	size_t idx = (head_ + capacity_ - 1) % capacity_;
	return buffer_[idx];
}

template<typename T>
const std::vector<T>& CircularBuffer<T>::getBuffer() const {
	return buffer_;
}

template<typename T>
bool CircularBuffer<T>::isEmpty() const {
	return size_ == 0;
}

template<typename T>
bool CircularBuffer<T>::isFull() const {
	return size_ == capacity_;
}

template<typename T>
size_t CircularBuffer<T>::size() const {
	return size_;
}

template<typename T>
size_t CircularBuffer<T>::capacity() const {
	return capacity_;
}

} // namespace Msg

#endif
