#include "test_allocator.h"

#include <cstdlib>
#include <limits>
#include <new>

std::size_t test_allocator::throw_countdown_ = std::numeric_limits<std::size_t>::max();

void* test_allocator::test_allocate(std::size_t count, std::align_val_t alignment)
{
    if (--throw_countdown_ == 0)
    {
        throw std::bad_alloc();
    }

#ifndef _MSC_VER
    void* ptr = std::aligned_alloc(static_cast<std::size_t>(alignment), count);
#else
    void* ptr = _aligned_malloc(count, static_cast<std::size_t>(alignment));
#endif
    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void test_allocator::test_deallocate(void* ptr) noexcept
{
#ifndef _MSC_VER
    std::free(ptr);
#else
    _aligned_free(ptr);
#endif
}

void* operator new(size_t count)
{
    return test_allocator::test_allocate(count, static_cast<std::align_val_t>(1));
}

void* operator new(size_t count, std::align_val_t alignment)
{
    return test_allocator::test_allocate(count, alignment);
}

void* operator new[](size_t count)
{
    return test_allocator::test_allocate(count, static_cast<std::align_val_t>(1));
}

void* operator new[](size_t count, std::align_val_t alignment)
{
    return test_allocator::test_allocate(count, alignment);
}

void operator delete(void* ptr) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] std::align_val_t al) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] size_t sz) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] size_t sz, [[maybe_unused]] std::align_val_t al) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] std::align_val_t al) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] size_t sz) noexcept
{
    test_allocator::test_deallocate(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] size_t sz, [[maybe_unused]] std::align_val_t al) noexcept
{
    test_allocator::test_deallocate(ptr);
}
