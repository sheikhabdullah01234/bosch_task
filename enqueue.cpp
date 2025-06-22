#ifndef ENQUEUE_CPP
#define ENQUEUE_CPP

#include "enqueue.h"

/**
 * @brief Construct a new safe queue object.
 * 
 * @tparam T The type of elements stored in the queue.
 * @param max_capacity The maximum number of elements the queue can hold.
 */
template <typename T>
safe_queue<T>::safe_queue(size_t max_capacity) 
    : maximum_capacity(max_capacity), current_size(0), first(0), last(0) {
    queue_data = new T[maximum_capacity];
}

/**
 * @brief Destroy the safe queue object.
 * 
 * @tparam T The type of elements stored in the queue.
 */
template <typename T>
safe_queue<T>::~safe_queue() {
    delete[] queue_data;
}

/**
 * @brief Get the current number of elements in the queue.
 * 
 * @tparam T The type of elements stored in the queue.
 * @return size_t The number of elements currently in the queue.
 */
template <typename T>
size_t safe_queue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex_sync);
    return current_size;
}

/**
 * @brief Check if the queue is empty.
 * 
 * @tparam T The type of elements stored in the queue.
 * @return true If the queue is empty.
 * @return false If the queue contains elements.
 */
template <typename T>
bool safe_queue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_sync);
    return current_size == 0;
}

/**
 * @brief Check if the queue is full.
 * 
 * @tparam T The type of elements stored in the queue.
 * @return true If the queue has reached maximum capacity.
 * @return false If the queue can accept more elements.
 */
template <typename T>
bool safe_queue<T>::full() const {
    std::lock_guard<std::mutex> lock(mutex_sync);
    return current_size == maximum_capacity;
}

/**
 * @brief Push an item into the queue (blocking).
 * 
 * @tparam T The type of elements stored in the queue.
 * @param item The item to push into the queue.
 */
template <typename T>
void safe_queue<T>::push(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_sync);
    is_full.wait(lock, [this]() { return current_size < maximum_capacity; });
    
    queue_data[last] = item;
    last = (last + 1) % maximum_capacity;
    ++current_size;
    
    is_empty.notify_one();
}

/**
 * @brief Push an item into the queue with timeout.
 * 
 * @tparam T The type of elements stored in the queue.
 * @param item The item to push into the queue.
 * @param timeout Maximum time to wait for space to become available.
 * @return true If the item was successfully pushed.
 * @throws std::runtime_error If the timeout expires before space becomes available.
 */
template <typename T>
bool safe_queue<T>::push(const T& item, const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(mutex_sync);
    
    if (!is_full.wait_for(lock, timeout, [this]() { return current_size < maximum_capacity; })) {
        throw std::runtime_error("Push timeout - queue is full");
    }
    
    queue_data[last] = item;
    last = (last + 1) % maximum_capacity;
    ++current_size;
    
    is_empty.notify_one();
    return true;
}

/**
 * @brief Pop an item from the queue (blocking).
 * 
 * @tparam T The type of elements stored in the queue.
 * @return T The popped item.
 */
template <typename T>
T safe_queue<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_sync);
    is_empty.wait(lock, [this]() { return current_size > 0; });
    
    T item = queue_data[first];
    first = (first + 1) % maximum_capacity;
    --current_size;
    
    is_full.notify_one();
    return item;
}

/**
 * @brief Pop an item from the queue with timeout.
 * 
 * @tparam T The type of elements stored in the queue.
 * @param item Reference to store the popped item.
 * @param timeout Maximum time to wait for an item to become available.
 * @return true If an item was successfully popped.
 * @throws std::runtime_error If the timeout expires before an item becomes available.
 */
template <typename T>
bool safe_queue<T>::pop(T& item, const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(mutex_sync);
    
    if (!is_empty.wait_for(lock, timeout, [this]() { return current_size > 0; })) {
        throw std::runtime_error("Pop timeout - queue is empty");
    }
    
    item = queue_data[first];
    first = (first + 1) % maximum_capacity;
    --current_size;
    
    is_full.notify_one();
    return true;
}

#endif