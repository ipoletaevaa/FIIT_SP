#include "../include/allocator_global_heap.h"
#include <cstdlib>
#include <new>
#include <mutex>
#include <map>
#include <memory_resource>

class allocator_global_heap::impl {
public:
    std::mutex m_mutex;
    std::map<void*, size_t> m_allocations;
    
    impl() = default;
};

allocator_global_heap::allocator_global_heap() 
    : m_impl(new impl()) {
}

allocator_global_heap::~allocator_global_heap() {
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    for (auto& [ptr, size] : m_impl->m_allocations) {
        std::free(ptr);
    }
    delete m_impl;
}

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other)
    : m_impl(new impl()) {
}

allocator_global_heap& allocator_global_heap::operator=(const allocator_global_heap &other) {
    return *this;
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept
    : m_impl(other.m_impl) {
    other.m_impl = nullptr;
}

allocator_global_heap& allocator_global_heap::operator=(allocator_global_heap &&other) noexcept {
    if (this != &other) {
        delete m_impl;
        m_impl = other.m_impl;
        other.m_impl = nullptr;
    }
    return *this;
}

void* allocator_global_heap::do_allocate_sm(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    
    void* ptr = std::malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    
    m_impl->m_allocations[ptr] = size;
    return ptr;
}

void allocator_global_heap::do_deallocate_sm(void *at) {
    if (at == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    
    auto it = m_impl->m_allocations.find(at);
    if (it != m_impl->m_allocations.end()) {
        m_impl->m_allocations.erase(it);
    }
    
    std::free(at);
}

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    return this == &other;
}