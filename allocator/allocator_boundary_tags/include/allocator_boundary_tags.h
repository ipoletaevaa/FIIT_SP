#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H

#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <pp_allocator.h>
#include <iterator>
#include <mutex>
#include <cstddef>

class allocator_boundary_tags final :
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:
    void *_trusted_memory;

    struct metadata_block {
        size_t block_size;
        metadata_block* prev_busy;
        metadata_block* next_busy;
        void *owner_allocator;
    };

    struct memory_header {
        std::pmr::memory_resource *parent_resource;
        allocator_with_fit_mode::fit_mode current_mode;
        size_t memory_size;
        std::mutex access_lock;
        metadata_block* first_busy;
    };

    struct free_spot {
        void* location;
        size_t available;
        metadata_block* left;
        metadata_block* right;
    };

    static constexpr const size_t allocator_metadata_size = sizeof(memory_header);
    static constexpr const size_t occupied_block_metadata_size = sizeof(metadata_block);
    static constexpr const size_t free_block_metadata_size = 0;

    static inline memory_header* get_header(void* mem) noexcept {
        return reinterpret_cast<memory_header*>(mem);
    }

    inline std::mutex& get_mutex() const noexcept {
        return reinterpret_cast<memory_header*>(_trusted_memory)->access_lock;
    }

    static inline std::byte* get_blocks_begin_ptr(void* mem) noexcept {
        return reinterpret_cast<std::byte*>(mem) + allocator_metadata_size;
    }

    static inline std::byte* get_end(void* mem) noexcept {
        return reinterpret_cast<std::byte*>(mem) + get_header(mem)->memory_size;
    }

    free_spot find_first_gap(size_t req_bytes) const;
    free_spot find_best_gap(size_t req_bytes) const;
    free_spot find_worst_gap(size_t req_bytes) const;

    static inline size_t calc_hole_size(void* busy_ref, void* mem_base) noexcept;

public:
    
    ~allocator_boundary_tags() override;
    
    allocator_boundary_tags(allocator_boundary_tags const &other);
    
    allocator_boundary_tags &operator=(allocator_boundary_tags const &other);
    
    allocator_boundary_tags(allocator_boundary_tags &&other) noexcept;
    
    allocator_boundary_tags &operator=(allocator_boundary_tags &&other) noexcept;

    explicit allocator_boundary_tags(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

private:
    
    [[nodiscard]] void *do_allocate_sm(size_t bytes) override;
    
    void do_deallocate_sm(void *at) override;

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

public:
    
    inline void set_fit_mode(allocator_with_fit_mode::fit_mode mode) override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    class boundary_iterator
    {
        void* _busy_ref;
        bool _is_busy;
        void* _mem_base;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = std::ptrdiff_t;

        bool operator==(const boundary_iterator&) const noexcept;
        bool operator!=(const boundary_iterator&) const noexcept;

        boundary_iterator& operator++() & noexcept;
        boundary_iterator& operator--() & noexcept;

        boundary_iterator operator++(int n);
        boundary_iterator operator--(int n);

        size_t size() const noexcept;
        bool occupied() const noexcept;

        void* operator*() const noexcept;
        void* get_ptr() const noexcept;

        boundary_iterator();
        boundary_iterator(void* trusted);

        metadata_block *get_prev_occupied() const noexcept;
        metadata_block *get_next_occupied() const noexcept;
    };

    friend class boundary_iterator;

    boundary_iterator begin() const noexcept;
    boundary_iterator end() const noexcept;
};

#endif