#ifndef __HIGH_PERFORMANCE_ORDER_BOOK_HPP__
#define __HIGH_PERFORMANCE_ORDER_BOOK_HPP__

#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <deque>
#include "Order.hpp"

namespace NSOrderMatching {

// Forward declaration
class MemoryPool;

/**
 * High-performance order book implementation using a flat array and memory pooling
 * for improved cache locality and reduced allocation overhead
 */
class HighPerformanceOrderBook {
public:
    HighPerformanceOrderBook(size_t initialCapacity = INIT_ORDER_BOOK_SIZE);
    ~HighPerformanceOrderBook();
    
    // Non-copyable, non-movable
    HighPerformanceOrderBook(const HighPerformanceOrderBook&) = delete;
    HighPerformanceOrderBook& operator=(const HighPerformanceOrderBook&) = delete;
    HighPerformanceOrderBook(HighPerformanceOrderBook&&) = delete;
    HighPerformanceOrderBook& operator=(HighPerformanceOrderBook&&) = delete;
    
    // Add a new order to the book
    unsigned long addOrder(Order&& order);
    
    // Get order by ID with bounds checking
    const Order& getOrder(unsigned long orderId) const;
    Order& getOrder(unsigned long orderId);
    
    // Update order status
    void updateOrderStatus(unsigned long orderId, OrderStatus status);
    
    // Get current size
    size_t size() const noexcept;
    
    // Get capacity
    size_t capacity() const noexcept;
    
    // Reserve capacity
    void reserve(size_t newCapacity);
    
    // Iterators for accessing all orders
    // Using a proxy iterator that respects thread safety
    class OrderIterator;
    OrderIterator begin() const;
    OrderIterator end() const;
    
    // Get orders for a specific stock
    std::vector<std::reference_wrapper<Order>> getOrdersByStock(const std::string& stock);
    
    // Thread-safe access for multi-threaded processing
    template<typename Func>
    void forEachOrder(Func func);
    
private:
    // The actual storage for orders - contiguous memory for better cache locality
    std::vector<Order> orders_;
    
    // Index by stock for efficient lookup
    std::unordered_map<std::string, std::vector<unsigned long>> stockIndex_;
    
    // Memory pool for order allocations to reduce memory fragmentation
    std::unique_ptr<MemoryPool> memPool_;
    
    // Thread safety
    mutable std::shared_mutex mutex_;
    
    // Current order count
    size_t orderCount_ = 0;
};

// Memory pool for efficient order object allocation
class MemoryPool {
public:
    MemoryPool(size_t objectSize, size_t initialCapacity);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
private:
    const size_t objectSize_;
    std::deque<void*> freeList_;
    std::vector<std::unique_ptr<char[]>> blocks_;
    std::mutex mutex_;
};

// Iterator implementation for HighPerformanceOrderBook
class HighPerformanceOrderBook::OrderIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Order;
    using difference_type = std::ptrdiff_t;
    using pointer = const Order*;
    using reference = const Order&;
    
    OrderIterator(const HighPerformanceOrderBook* book, size_t index);
    
    reference operator*() const;
    pointer operator->() const;
    OrderIterator& operator++();
    OrderIterator operator++(int);
    bool operator==(const OrderIterator& other) const;
    bool operator!=(const OrderIterator& other) const;
    
private:
    const HighPerformanceOrderBook* book_;
    size_t index_;
};

} // namespace NSOrderMatching

#endif // __HIGH_PERFORMANCE_ORDER_BOOK_HPP__