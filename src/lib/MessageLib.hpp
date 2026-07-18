#pragma once

#include <mutex>
#include <vector>
#include <stdexcept> // For std::out_of_range
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <type_traits>

#include "types.h"

namespace Msg {

    template <typename T>
    class CircularBuffer {
    public:
        // Constructor: Initializes the buffer with a given capacity
        explicit CircularBuffer(size_t capacity);

        CircularBuffer(const CircularBuffer&) = delete;
        CircularBuffer& operator=(const CircularBuffer&) = delete;
        CircularBuffer(CircularBuffer&&) = delete;
        CircularBuffer& operator=(CircularBuffer&&) = delete;
    
        ~CircularBuffer() = default;

        void killProcess();

        // Adds an element to the buffer (overwrites oldest if full)
        void push(const T& item);

        // Removes and returns the oldest element from the buffer
        void pop();

        // Clears the buffer completely
        void flush();

        // Returns a reference to the element at a specific index relative to the head
        // (0 is the oldest element, size-1 is the newest)
        T& operator[](size_t index);

        // Const version of operator[]
        const T& operator[](size_t index) const;

        /**
         * @brief Peek at an element at a specific index without removing it from the buffer
         * 
         * @param index 
         * @return T& 
         */
        T& peek(size_t index);

        /**
         * @brief If the buffer is empty, this will block until an item is added, then return a reference to the newest item (head)
         * 
         * @param timeout Timeout in milliseconds (-1 for infinite)
         * @return T& 
         */
        T& getHead(int timeout=-1);

        // Const version of getHead
        const T& getHead() const;

        const std::vector<T>& getBuffer() const;

        // Checks if the buffer is empty
        bool isEmpty() const;

        // Checks if the buffer is full
        bool isFull() const;

        // Returns the current number of elements in the buffer
        size_t size() const;

        // Returns the maximum capacity of the buffer
        size_t capacity() const;

    private:
        std::vector<T> buffer_;             // Underlying storage for the buffer
        size_t head_ = 0;                   // Index of the next available slot for writing
        size_t tail_ = 0;                   // Index of the oldest element (next to be read)
        size_t size_ = 0;                   // Current number of elements in the buffer
        size_t count = 0;                   // Number of items in buffer
        size_t capacity_ = 0;               // Maximum capacity of the buffer
        std::condition_variable m_BufferCv; // Condition variable for synchronization
        std::mutex bufferMutex;             // Circular buffer mutex
    };
};

#define MESSAGE_LIB_IMPL
#include "MessageLib.cpp"
#undef MESSAGE_LIB_IMPL
