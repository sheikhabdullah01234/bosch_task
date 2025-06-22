#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include "enqueue.h"

class SafeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a queue with capacity 5 for most tests
        q = std::make_unique<safe_queue<int>>(5);
    }

    void TearDown() override {
        q.reset();
    }

    std::unique_ptr<safe_queue<int>> q;
};

// Basic functionality tests
TEST_F(SafeQueueTest, InitialState) {
    EXPECT_TRUE(q->empty());
    EXPECT_FALSE(q->full());
    EXPECT_EQ(q->size(), 0);
}

TEST_F(SafeQueueTest, PushAndPop) {
    q->push(42);
    EXPECT_FALSE(q->empty());
    EXPECT_EQ(q->size(), 1);
    
    int val = q->pop();
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(q->empty());
}

TEST_F(SafeQueueTest, PushUntilFull) {
    for (int i = 1; i <= 5; ++i) {
        q->push(i);
        EXPECT_EQ(q->size(), i);
    }
    
    EXPECT_TRUE(q->full());
    EXPECT_EQ(q->size(), 5);
}

TEST_F(SafeQueueTest, PopUntilEmpty) {
    for (int i = 1; i <= 5; ++i) {
        q->push(i);
    }
    
    for (int i = 1; i <= 5; ++i) {
        EXPECT_EQ(q->pop(), i);
        EXPECT_EQ(q->size(), 5 - i);
    }
    
    EXPECT_TRUE(q->empty());
}

// Timeout tests
TEST_F(SafeQueueTest, PushWithTimeoutSuccess) {
    // Should succeed immediately since queue is empty
    EXPECT_TRUE(q->push(1, std::chrono::milliseconds(100)));
    EXPECT_EQ(q->size(), 1);
}

TEST_F(SafeQueueTest, PushWithTimeoutFailure) {
    // Fill the queue
    for (int i = 0; i < 5; ++i) {
        q->push(i);
    }
    
    // Try to push to full queue with short timeout
    EXPECT_THROW(q->push(6, std::chrono::milliseconds(50)), std::runtime_error);
}

TEST_F(SafeQueueTest, PopWithTimeoutSuccess) {
    q->push(42);
    int val;
    EXPECT_TRUE(q->pop(val, std::chrono::milliseconds(100)));
    EXPECT_EQ(val, 42);
}

TEST_F(SafeQueueTest, PopWithTimeoutFailure) {
    int val;
    EXPECT_THROW(q->pop(val, std::chrono::milliseconds(50)), std::runtime_error);
}

// Thread safety tests
TEST_F(SafeQueueTest, ConcurrentPushPop) {
    const int num_items = 1000;
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < num_items; ++i) {
            q->push(i);
            produced++;
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < num_items; ++i) {
            int val = q->pop();
            consumed++;
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(produced, num_items);
    EXPECT_EQ(consumed, num_items);
    EXPECT_TRUE(q->empty());
}

TEST_F(SafeQueueTest, MultipleProducersMultipleConsumers) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 250;
    const int total_items = num_producers * items_per_producer;
    
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Create producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < items_per_producer; ++j) {
                q->push(j);
                produced++;
            }
        });
    }
    
    // Create consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int val = q->pop();
                consumed++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    
    EXPECT_EQ(produced, total_items);
    EXPECT_EQ(consumed, total_items);
    EXPECT_TRUE(q->empty());
}

TEST_F(SafeQueueTest, FullEmptyStressTest) {
    const int num_operations = 1000;
    std::atomic<bool> done(false);
    
    // Producer thread - tries to keep queue full
    std::thread producer([&]() {
        for (int i = 0; i < num_operations; ++i) {
            q->push(i);
        }
        done = true;
    });
    
    // Consumer thread - tries to keep queue empty
    std::thread consumer([&]() {
        while (!done || !q->empty()) {
            try {
                int val;
                if (q->pop(val, std::chrono::milliseconds(10))) {
                    // Successfully popped
                }
            } catch (const std::runtime_error&) {
                // Timeout - queue was empty
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_TRUE(q->empty());
}

// Edge case tests
TEST_F(SafeQueueTest, ZeroCapacityQueue) {
    safe_queue<int> zero_queue(0);
    
    EXPECT_TRUE(zero_queue.empty());
    EXPECT_TRUE(zero_queue.full());
    
    // Push should fail immediately
    EXPECT_THROW(zero_queue.push(1, std::chrono::milliseconds(10)), std::runtime_error);
    
    // Pop should fail immediately
    int val;
    EXPECT_THROW(zero_queue.pop(val, std::chrono::milliseconds(10)), std::runtime_error);
}

TEST_F(SafeQueueTest, SingleCapacityQueue) {
    safe_queue<int> single_queue(1);
    
    // Should be able to push one item
    single_queue.push(42);
    EXPECT_TRUE(single_queue.full());
    
    // Second push should fail
    EXPECT_THROW(single_queue.push(43, std::chrono::milliseconds(10)), std::runtime_error);
    
    // Pop should get the item
    int val = single_queue.pop();
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(single_queue.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}