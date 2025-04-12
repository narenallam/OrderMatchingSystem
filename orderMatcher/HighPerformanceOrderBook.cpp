#include "HighPerformanceOrderBook.hpp"
#include <stdexcept>
#include <algorithm>

namespace NSOrderMatching {

// Store block sizes
struct BlockInfo {
    size_t size;
};

// MemoryPool implementation
MemoryPool::MemoryPool(size_t objectSize, size_t initialCapacity)
    : objectSize_(objectSize) {
    // Allocate initial block
    auto initialBlock = std::make_unique<char[]>(objectSize_ * initialCapacity);
    
    // Initialize free list with all objects in the block
    for (size_t i = 0; i < initialCapacity; ++i) {
        void* ptr = initialBlock.get() + (i * objectSize_);
        freeList_.push_back(ptr);
    }
    
    // Store the block
    blocks_.push_back(std::move(initialBlock));
}

MemoryPool::~MemoryPool() {
    // No need to manually free blocks as they'll be cleaned up by unique_ptr
}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (freeList_.empty()) {
        // Allocate a new block with twice the size of the last block
        size_t newBlockSize = blocks_.empty() ? 1024 : 2048;  // Fixed initial block sizes
        auto newBlock = std::make_unique<char[]>(objectSize_ * newBlockSize);
        
        // Add all objects from the new block to the free list
        for (size_t i = 0; i < newBlockSize; ++i) {
            void* ptr = newBlock.get() + (i * objectSize_);
            freeList_.push_back(ptr);
        }
        
        blocks_.push_back(std::move(newBlock));
    }
    
    // Get a free object
    void* result = freeList_.front();
    freeList_.pop_front();
    return result;
}

void MemoryPool::deallocate(void* ptr) {
    if (ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        freeList_.push_back(ptr);
    }
}

// HighPerformanceOrderBook implementation
HighPerformanceOrderBook::HighPerformanceOrderBook(size_t initialCapacity) {
    orders_.reserve(initialCapacity);
    memPool_ = std::make_unique<MemoryPool>(sizeof(Order), initialCapacity);
}

HighPerformanceOrderBook::~HighPerformanceOrderBook() = default;

unsigned long HighPerformanceOrderBook::addOrder(Order&& order) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Store the order ID
    unsigned long orderId = orderCount_++;
    
    // Ensure capacity
    if (orderId >= orders_.size()) {
        size_t newCapacity = orders_.size() == 0 ? INIT_ORDER_BOOK_SIZE : orders_.size() * 2;
        orders_.reserve(newCapacity);
        while (orders_.size() <= orderId) {
            orders_.emplace_back(); // Default construct
        }
    }
    
    // Store the order
    orders_[orderId] = std::move(order);
    orders_[orderId].orderId = orderId;
    
    // Update index
    stockIndex_[orders_[orderId].stock].push_back(orderId);
    
    return orderId;
}

const Order& HighPerformanceOrderBook::getOrder(unsigned long orderId) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (orderId >= orders_.size()) {
        throw std::out_of_range("Order ID out of range");
    }
    
    return orders_[orderId];
}

Order& HighPerformanceOrderBook::getOrder(unsigned long orderId) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (orderId >= orders_.size()) {
        throw std::out_of_range("Order ID out of range");
    }
    
    return orders_[orderId];
}

void HighPerformanceOrderBook::updateOrderStatus(unsigned long orderId, OrderStatus status) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (orderId >= orders_.size()) {
        throw std::out_of_range("Order ID out of range");
    }
    
    orders_[orderId].status = status;
}

size_t HighPerformanceOrderBook::size() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return orderCount_;
}

size_t HighPerformanceOrderBook::capacity() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return orders_.capacity();
}

void HighPerformanceOrderBook::reserve(size_t newCapacity) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    orders_.reserve(newCapacity);
}

std::vector<std::reference_wrapper<Order>> HighPerformanceOrderBook::getOrdersByStock(const std::string& stock) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::reference_wrapper<Order>> result;
    auto it = stockIndex_.find(stock);
    
    if (it != stockIndex_.end()) {
        result.reserve(it->second.size());
        for (auto orderId : it->second) {
            result.emplace_back(std::ref(orders_[orderId]));
        }
    }
    
    return result;
}

// Removed the forEachOrder template implementation since it's already in the header

// OrderIterator implementation
HighPerformanceOrderBook::OrderIterator HighPerformanceOrderBook::begin() const {
    return OrderIterator(this, 0);
}

HighPerformanceOrderBook::OrderIterator HighPerformanceOrderBook::end() const {
    return OrderIterator(this, orderCount_);
}

HighPerformanceOrderBook::OrderIterator::OrderIterator(const HighPerformanceOrderBook* book, size_t index)
    : book_(book), index_(index) {
}

HighPerformanceOrderBook::OrderIterator::reference HighPerformanceOrderBook::OrderIterator::operator*() const {
    return book_->getOrder(index_);
}

HighPerformanceOrderBook::OrderIterator::pointer HighPerformanceOrderBook::OrderIterator::operator->() const {
    return &(book_->getOrder(index_));
}

HighPerformanceOrderBook::OrderIterator& HighPerformanceOrderBook::OrderIterator::operator++() {
    ++index_;
    return *this;
}

HighPerformanceOrderBook::OrderIterator HighPerformanceOrderBook::OrderIterator::operator++(int) {
    OrderIterator tmp(*this);
    ++(*this);
    return tmp;
}

bool HighPerformanceOrderBook::OrderIterator::operator==(const OrderIterator& other) const {
    return book_ == other.book_ && index_ == other.index_;
}

bool HighPerformanceOrderBook::OrderIterator::operator!=(const OrderIterator& other) const {
    return !(*this == other);
}

} // namespace NSOrderMatching