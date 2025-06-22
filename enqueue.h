#ifndef ENQUEUE_H
#define ENQUEUE_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

/**
 * @brief A thread-safe queue implementation with fixed capacity and timeout support.
 * 
 * @tparam T The type of elements stored in the queue.
 * 
 * This class provides a thread-safe FIFO queue with the following features:
 * - Fixed maximum capacity
 * - Blocking and non-blocking operations
 * - Timeout support for push and pop operations
 * - Thread-safe size checking
 * - Exception safety
 */
template <typename T>
class safe_queue {
private:
    T* queue_data;                          ///< Dynamic array to store elements
    size_t maximum_capacity;                ///< Maximum capacity of the queue
    size_t current_size;                    ///< Current number of elements
    size_t first;                           ///< Index of the first element
    size_t last;                            ///< Index where next element will be inserted
    mutable std::mutex mutex_sync;          ///< Mutex for synchronization
    std::condition_variable is_full;        ///< Condition variable for push operations
    std::condition_variable is_empty;       ///< Condition variable for pop operations

public:
    /**
     * @brief Construct a new safe queue object.
     * 
     * @param max_capacity The maximum number of elements the queue can hold.
     */
    explicit safe_queue(size_t max_capacity);
    
    /**
     * @brief Destroy the safe queue object.
     * 
     * Releases all allocated resources.
     */
    ~safe_queue();

    /**
     * @brief Get the current number of elements in the queue.
     * 
     * @return size_t The number of elements currently in the queue.
     */
    size_t size() const;

    /**
     * @brief Check if the queue is empty.
     * 
     * @return true If the queue is empty.
     * @return false If the queue contains elements.
     */
    bool empty() const;

    /**
     * @brief Check if the queue is full.
     * 
     * @return true If the queue has reached maximum capacity.
     * @return false If the queue can accept more elements.
     */
    bool full() const;

    /**
     * @brief Push an item into the queue (blocking).
     * 
     * @param item The item to push into the queue.
     * 
     * @note This method will block if the queue is full until space becomes available.
     */
    void push(const T& item);

    /**
     * @brief Push an item into the queue with timeout.
     * 
     * @param item The item to push into the queue.
     * @param timeout Maximum time to wait for space to become available.
     * @return true If the item was successfully pushed.
     * @throws std::runtime_error If the timeout expires before space becomes available.
     */
    bool push(const T& item, const std::chrono::milliseconds& timeout);

    /**
     * @brief Pop an item from the queue (blocking).
     * 
     * @return T The popped item.
     * 
     * @note This method will block if the queue is empty until an item becomes available.
     */
    T pop();

    /**
     * @brief Pop an item from the queue with timeout.
     * 
     * @param item Reference to store the popped item.
     * @param timeout Maximum time to wait for an item to become available.
     * @return true If an item was successfully popped.
     * @throws std::runtime_error If the timeout expires before an item becomes available.
     */
    bool pop(T& item, const std::chrono::milliseconds& timeout);

    // Disable copy and assignment
    safe_queue(const safe_queue&) = delete;            ///< Copy constructor is deleted
    safe_queue& operator=(const safe_queue&) = delete; ///< Assignment operator is deleted
};

#endif