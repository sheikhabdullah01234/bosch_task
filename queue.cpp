#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

template <typename T>
class safe_queue {
private:
    T* queue_size;                          // Dynamic array to store elements
    size_t maximum_capacity;                // Maximum maximum_capacity of the queue
    size_t enqueue_element;                 // Current number of elements
    size_t first;                           // Index of the first element
    size_t last;                            // Index where next element will be inserted
    std::mutex mutex_sync;                  // Mutex for synchronization
    std::condition_variable is_full;        // Condition variable for push
    std::condition_variable is_empty;       // Condition variable for pop

public:
    // Constructor with maximum maximum_capacity
    explicit safe_queue(size_t max_maximum_capacity) 
        : maximum_capacity(max_maximum_capacity), enqueue_element(0), first(0), last(0) {
        queue_size = new T[maximum_capacity];
    }

    // Destructor
    ~safe_queue() {
        delete[] queue_size;
    }

    // Get current number of elements in the queue
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_sync);
        return enqueue_element;
    }

    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_sync);
        return enqueue_element == 0;
    }

    // Check if queue is full
    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_sync);
        return enqueue_element == maximum_capacity;
    }

    // Push with no timeout (blocks indefinitely when full)
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_sync);
        is_full.wait(lock, [this]() { return enqueue_element < maximum_capacity; });
        
        queue_size[last] = item;
        last = (last + 1) % maximum_capacity;
        ++enqueue_element;
        
        is_empty.notify_one();
    }

    // Push with timeout (throws if timeout occurs)
    bool push(const T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_sync);
        
        if (!is_full.wait_for(lock, timeout, [this]() { return enqueue_element < maximum_capacity; })) {
            throw std::runtime_error("Push timeout - queue is full");
        }
        
        queue_size[last] = item;
        last = (last + 1) % maximum_capacity;
        ++enqueue_element;
        
        is_empty.notify_one();
        return true;
    }

    // Pop with no timeout (blocks indefinitely when empty)
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_sync);
        is_empty.wait(lock, [this]() { return enqueue_element > 0; });
        
        T item = queue_size[first];
        first = (first + 1) % maximum_capacity;
        --enqueue_element;
        
        is_full.notify_one();
        return item;
    }

    // Pop with timeout (throws if timeout occurs)
    bool pop(T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_sync);
        
        if (!is_empty.wait_for(lock, timeout, [this]() { return enqueue_element > 0; })) {
            throw std::runtime_error("Pop timeout - queue is empty");
        }
        
        item = queue_size[first];
        first = (first + 1) % maximum_capacity;
        --enqueue_element;
        
        is_full.notify_one();
        return true;
    }

    // Disable copy and assignment
    safe_queue(const safe_queue&) = delete;
    safe_queue& operator=(const safe_queue&) = delete;
};