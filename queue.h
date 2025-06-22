// ThreadSafeQueue.h
#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdexcept>
#include <memory>  // For std::unique_ptr in C++11/14 or later

template <typename T>
class ThreadSafeQueue {
private:
    std::unique_ptr<T[]> buffer;    // Safer alternative to raw pointer
    size_t capacity;
    size_t count = 0;
    size_t front = 0;
    size_t rear = 0;
    mutable std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;

public:
    explicit ThreadSafeQueue(size_t max_capacity);
    ~ThreadSafeQueue() = default;  // unique_ptr handles cleanup

    // Size and capacity queries
    size_t size() const;
    bool empty() const;
    bool full() const;

    // Push operations
    void push(const T& item);
    bool push(const T& item, const std::chrono::milliseconds& timeout);
    
    // Pop operations
    T pop();
    bool pop(T& item, const std::chrono::milliseconds& timeout);

    // Move operations (optional but recommended)
    ThreadSafeQueue(ThreadSafeQueue&&) noexcept = default;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) noexcept = default;

    // Delete copy operations
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
};

// Template implementation
template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(size_t max_capacity) 
    : capacity(max_capacity), buffer(std::make_unique<T[]>(max_capacity)) {}

template <typename T>
size_t ThreadSafeQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return count;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return count == 0;
}

template <typename T>
bool ThreadSafeQueue<T>::full() const {
    std::lock_guard<std::mutex> lock(mtx);
    return count == capacity;
}

template <typename T>
void ThreadSafeQueue<T>::push(const T& item) {
    std::unique_lock<std::mutex> lock(mtx);
    not_full.wait(lock, [this]() { return count < capacity; });
    
    buffer[rear] = item;
    rear = (rear + 1) % capacity;
    ++count;
    
    not_empty.notify_one();
}

template <typename T>
bool ThreadSafeQueue<T>::push(const T& item, const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(mtx);
    
    if (!not_full.wait_for(lock, timeout, [this]() { return count < capacity; })) {
        throw std::runtime_error("Push timeout - queue is full");
    }
    
    buffer[rear] = item;
    rear = (rear + 1) % capacity;
    ++count;
    
    not_empty.notify_one();
    return true;
}

template <typename T>
T ThreadSafeQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(mtx);
    not_empty.wait(lock, [this]() { return count > 0; });
    
    T item = buffer[front];
    front = (front + 1) % capacity;
    --count;
    
    not_full.notify_one();
    return item;
}

template <typename T>
bool ThreadSafeQueue<T>::pop(T& item, const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(mtx);
    
    if (!not_empty.wait_for(lock, timeout, [this]() { return count > 0; })) {
        throw std::runtime_error("Pop timeout - queue is empty");
    }
    
    item = buffer[front];
    front = (front + 1) % capacity;
    --count;
    
    not_full.notify_one();
    return true;
}

#endif // THREAD_SAFE_QUEUE_H