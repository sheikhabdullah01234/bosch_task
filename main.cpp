#include <iostream>
#include <thread>
#include <chrono>
#include "enqueue.h"

int main() {
    // Create a safe_queue of integers with capacity 5
    safe_queue<int> myQueue(5);

    // Test basic operations
    std::cout << "Queue empty? " << (myQueue.empty() ? "Yes" : "No") << std::endl;
    std::cout << "Queue size: " << myQueue.size() << std::endl;

    // Push some items
    std::cout << "\nPushing items..." << std::endl;
    for (int i = 1; i <= 5; ++i) {
        myQueue.push(i);
        std::cout << "Pushed: " << i << ", Size: " << myQueue.size() << std::endl;
    }

    // Check if full
    std::cout << "\nQueue full? " << (myQueue.full() ? "Yes" : "No") << std::endl;

    // Try pushing with timeout (should throw)
    try {
        std::cout << "\nTrying to push with timeout (should fail)..." << std::endl;
        myQueue.push(6, std::chrono::milliseconds(100));
    } catch (const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    // Pop items
    std::cout << "\nPopping items..." << std::endl;
    while (!myQueue.empty()) {
        int val = myQueue.pop();
        std::cout << "Popped: " << val << ", Size: " << myQueue.size() << std::endl;
    }

    // Try popping with timeout (should throw)
    try {
        std::cout << "\nTrying to pop with timeout (should fail)..." << std::endl;
        int val;
        myQueue.pop(val, std::chrono::milliseconds(100));
    } catch (const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    // Demonstrate multi-threaded usage
    std::cout << "\nStarting producer-consumer demo..." << std::endl;
    
    // Producer thread
    std::thread producer([&myQueue]() {
        for (int i = 10; i < 15; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            myQueue.push(i);
            std::cout << "Produced: " << i << std::endl;
        }
    });

    // Consumer thread
    std::thread consumer([&myQueue]() {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            int val = myQueue.pop();
            std::cout << "Consumed: " << val << std::endl;
        }
    });

    producer.join();
    consumer.join();

    std::cout << "\nDemo complete!" << std::endl;
    return 0;
}