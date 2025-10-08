#pragma once

#include <cstddef>
#include <new>

struct test_allocator
{
private:
    static std::size_t throw_countdown_;

public:
    static void* test_allocate(std::size_t count, std::align_val_t alignment);
    static void test_deallocate(void* ptr) noexcept;

    friend void reset_counts() noexcept;
    friend void allocate_countdown(std::size_t countdown) noexcept;
};
