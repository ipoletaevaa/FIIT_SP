#include <not_implemented.h>
#include "../include/allocator_boundary_tags.h"
#include <cstring>
#include <new>

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (_trusted_memory == nullptr) return;
    auto *hdr = get_header(_trusted_memory);

    std::pmr::memory_resource *parent = hdr->parent_resource;
    size_t total = hdr->memory_size;

    hdr->access_lock.~mutex();
    if (parent != nullptr){
        parent->deallocate(_trusted_memory, total, 1);
    }else{
        ::operator delete(_trusted_memory);
    }
    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(allocator_boundary_tags &&other) noexcept
{
    std::swap(_trusted_memory, other._trusted_memory);
    return *this;
}

allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size < occupied_block_metadata_size){
        throw std::bad_alloc();
    }

    size_t total_size = space_size + allocator_metadata_size;
    if (parent_allocator != nullptr){
        _trusted_memory = parent_allocator->allocate(total_size, 1);
    }else{
        _trusted_memory = ::operator new(total_size);
    }
    
    auto *hdr = get_header(_trusted_memory);
    new (&(hdr->access_lock)) std::mutex();
    hdr->parent_resource = parent_allocator;
    hdr->current_mode = allocate_fit_mode;
    hdr->memory_size = total_size;
    hdr->first_busy = nullptr;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(size_t size)
{
    std::lock_guard lock(get_mutex());
    auto *hdr = get_header(_trusted_memory);

    size_t req = size + occupied_block_metadata_size;

    free_spot spot = {nullptr, 0, nullptr, nullptr};
    switch (hdr->current_mode)
    {
        case allocator_with_fit_mode::fit_mode::first_fit:
            spot = find_first_gap(req);
            break;
        case allocator_with_fit_mode::fit_mode::the_best_fit:
            spot = find_best_gap(req);
            break;
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
            spot = find_worst_gap(req);
            break;
    }
    if (!spot.location){
        throw std::bad_alloc();
    }

    auto *blk = reinterpret_cast<metadata_block*>(spot.location);
    blk->prev_busy = spot.left;
    blk->next_busy = spot.right;
    blk->owner_allocator = this;

    blk->block_size = (spot.available >= req + occupied_block_metadata_size) ? size : spot.available - occupied_block_metadata_size;

    if (spot.left){
        spot.left->next_busy = blk;
    }else{
        hdr->first_busy = blk;
    }
    if (spot.right) spot.right->prev_busy = blk;
    
    return reinterpret_cast<std::byte*>(blk) + occupied_block_metadata_size;
}

allocator_boundary_tags::free_spot allocator_boundary_tags::find_first_gap(size_t req) const {
    for (auto it = begin(); it != end(); ++it)
    {
        if (!it.occupied() && it.size() >= req)
        {
            return { *it, it.size(), it.get_prev_occupied(), it.get_next_occupied() };
        }
    }
    return { nullptr, 0, nullptr, nullptr };
}

allocator_boundary_tags::free_spot allocator_boundary_tags::find_best_gap(size_t req) const {
    boundary_iterator best = end();
    for (auto it = begin(); it != end(); ++it)
    {
        if ((!it.occupied() && it.size() >= req) && (best == end() || best.size() > it.size()))
        {
            best = it;
        }
    }
    if (best == end()) return { nullptr, 0, nullptr, nullptr };
    return { *best, best.size(), best.get_prev_occupied(), best.get_next_occupied() };
}

allocator_boundary_tags::free_spot allocator_boundary_tags::find_worst_gap(size_t req) const {
    boundary_iterator worst = end();
    for (auto it = begin(); it != end(); ++it)
    {
        if ((!it.occupied() && it.size() >= req) && (worst == end() || worst.size() < it.size()))
        {
            worst = it;
        }
    }
    if (worst == end()) return { nullptr, 0, nullptr, nullptr };
    return { *worst, worst.size(), worst.get_prev_occupied(), worst.get_next_occupied() };
}

void allocator_boundary_tags::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    std::lock_guard lock(get_mutex());

    void* block = static_cast<std::byte*>(at) - occupied_block_metadata_size;
    void* start = _trusted_memory;
    void* end = get_end(_trusted_memory);
    if (block < start || block >= end) 
        throw std::logic_error("pointer out of bounds");

    auto *blk = reinterpret_cast<metadata_block*>(block);
    if (blk->owner_allocator != this){ 
        throw std::logic_error("block does not belong to this allocator");
    }

    auto *hdr = get_header(_trusted_memory);
    auto *left = blk->prev_busy;
    auto *right = blk->next_busy;

    if (left != nullptr) {
        left->next_busy = right;
    }else{
        hdr->first_busy = right;
    }
    if (right != nullptr) 
        right->prev_busy = left;
}

inline void allocator_boundary_tags::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex());
    get_header(_trusted_memory)->current_mode = mode;
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    std::lock_guard lock(get_mutex());
    return get_blocks_info_inner();
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return {};
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> result;
    for (auto it = begin(); it != end(); ++it)
    {
        result.push_back({ it.size(), it.occupied() });
    }
    return result;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    std::lock_guard lock(other.get_mutex());

    auto *src_hdr = get_header(other._trusted_memory);
    size_t total = src_hdr->memory_size;
    auto *parent = src_hdr->parent_resource;

    if (parent != nullptr){
        _trusted_memory = parent->allocate(total, 1);
    } else{
        _trusted_memory = ::operator new(total);
    }

    std::memcpy(_trusted_memory, other._trusted_memory, total);

    auto *hdr = get_header(_trusted_memory);
    new(&(hdr->access_lock)) std::mutex();

    if (hdr->first_busy != nullptr){
        size_t shift = reinterpret_cast<std::byte*>(hdr->first_busy) - reinterpret_cast<std::byte*>(other._trusted_memory);
        hdr->first_busy = reinterpret_cast<metadata_block*>(reinterpret_cast<std::byte*>(_trusted_memory) + shift);
    }

    auto *cur = hdr->first_busy;
    while (cur != nullptr){
        cur->owner_allocator = this;

        if (cur->next_busy != nullptr){
            size_t n_shift = reinterpret_cast<std::byte*>(cur->next_busy) - reinterpret_cast<std::byte*>(other._trusted_memory);
            cur->next_busy = reinterpret_cast<metadata_block*>(reinterpret_cast<std::byte*>(_trusted_memory) + n_shift);
        }

        auto *nxt = cur->next_busy;
        if (nxt != nullptr) nxt->prev_busy = cur;
        cur = nxt;
    }
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    allocator_boundary_tags tmp(other);
    std::swap(_trusted_memory, tmp._trusted_memory);
    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

bool allocator_boundary_tags::boundary_iterator::operator==(const boundary_iterator &other) const noexcept
{
    return _busy_ref == other._busy_ref && ((_is_busy == other._is_busy && _mem_base == other._mem_base) || _busy_ref == nullptr);
}

bool allocator_boundary_tags::boundary_iterator::operator!=(const boundary_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_is_busy){
        auto *blk = reinterpret_cast<metadata_block*>(_busy_ref);
        auto *nxt = reinterpret_cast<std::byte*>(_busy_ref) + blk->block_size + occupied_block_metadata_size;
        auto *nxt_busy = blk->next_busy;
        if (nxt_busy != reinterpret_cast<metadata_block*>(nxt) && nxt < get_end(_mem_base)){
            _is_busy = false;
        }else{
            _busy_ref = reinterpret_cast<void*>(nxt_busy);
            _is_busy = (_busy_ref != nullptr);
        }
    }else{
        if (_busy_ref == _mem_base){
            _busy_ref = get_header(_mem_base)->first_busy;
        }else{
            _busy_ref = reinterpret_cast<metadata_block*>(_busy_ref)->next_busy;
        }
        _is_busy = (_busy_ref != nullptr);
    }
    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_is_busy)
    {
        auto *blk = reinterpret_cast<metadata_block*>(_busy_ref);
        auto *left = blk->prev_busy;

        if (left == nullptr)
        {
            void* start = get_blocks_begin_ptr(_mem_base);
            if (_busy_ref != start)
            {
                _is_busy = false;
                _busy_ref = _mem_base;
            }
        }
        else
        {
            auto* left_end = reinterpret_cast<std::byte*>(left) + left->block_size + occupied_block_metadata_size;
            if (left_end != reinterpret_cast<std::byte*>(_busy_ref))
            {
                _is_busy = false;
                _busy_ref = left;
            }
            else
            {
                _busy_ref = left;
                _is_busy = true;
            }
        }
    }
    else
    {
        if (_busy_ref != _mem_base) _is_busy = true;
    }
    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_busy_ref == nullptr) return 0;
    if (_is_busy){
        auto *blk = reinterpret_cast<metadata_block*>(_busy_ref);
        return blk->block_size + occupied_block_metadata_size;
    }else{
        return calc_hole_size(_busy_ref, _mem_base);
    }
}

size_t allocator_boundary_tags::calc_hole_size(void *busy_ref, void* mem_base) noexcept {
    if (busy_ref == mem_base){
        auto *first = reinterpret_cast<void*>(get_header(mem_base)->first_busy);
        if (first == nullptr){
            return get_end(mem_base) - get_blocks_begin_ptr(mem_base);
        }else{
            return reinterpret_cast<std::byte*>(first) - get_blocks_begin_ptr(mem_base);
        }
    }else{
        auto *nxt = reinterpret_cast<metadata_block*>(busy_ref)->next_busy;
        auto *blk = reinterpret_cast<metadata_block*>(busy_ref);
        if (nxt == nullptr){
            return get_end(mem_base) - (reinterpret_cast<std::byte*>(busy_ref) + blk->block_size + occupied_block_metadata_size);
        }else{
            return reinterpret_cast<std::byte*>(nxt) - (reinterpret_cast<std::byte*>(busy_ref) + blk->block_size + occupied_block_metadata_size);
        }
    }
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _is_busy;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    if (_is_busy){
        return _busy_ref;
    }else{
        if (_busy_ref == _mem_base)
        {
            return get_blocks_begin_ptr(_mem_base);
        }
        auto *blk = reinterpret_cast<metadata_block*>(_busy_ref);
        return reinterpret_cast<std::byte*>(_busy_ref) + blk->block_size + occupied_block_metadata_size;
    }
}

allocator_boundary_tags::metadata_block *allocator_boundary_tags::boundary_iterator::get_prev_occupied() const noexcept
{
    if (_busy_ref == nullptr) return nullptr;
    if (_is_busy) return reinterpret_cast<metadata_block*>(_busy_ref)->prev_busy;
    if (_busy_ref == _mem_base) return nullptr; 
    return reinterpret_cast<metadata_block*>(_busy_ref);
}

allocator_boundary_tags::metadata_block *allocator_boundary_tags::boundary_iterator::get_next_occupied() const noexcept
{
    if (_busy_ref == nullptr) return nullptr;
    if (_is_busy) return reinterpret_cast<metadata_block*>(_busy_ref)->next_busy;
    if (_busy_ref == _mem_base) return get_header(_mem_base)->first_busy;
    return reinterpret_cast<metadata_block*>(_busy_ref)->next_busy;
}

allocator_boundary_tags::boundary_iterator::boundary_iterator() : _busy_ref(nullptr), _is_busy(false), _mem_base(nullptr) {}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted) : _mem_base(trusted)
{
    if (!_mem_base) {
        _is_busy = false;
        _busy_ref = nullptr;
    } else {
        auto *hdr = get_header(_mem_base);
        void* start = reinterpret_cast<void*>(get_blocks_begin_ptr(_mem_base));

        if (hdr->first_busy == start) {
            _is_busy = true;
            _busy_ref = start;
        } else {
            _is_busy = false;
            _busy_ref = _mem_base;
        }
    }
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _busy_ref;
}