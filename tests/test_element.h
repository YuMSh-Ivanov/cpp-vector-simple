#pragma once

#include <cstddef>
#include <ostream>

struct test_element
{
private:
    int value_;

    static std::size_t objects_count_;
    static std::size_t copy_count_;

    static std::size_t throw_copy_countdown_;

    static void decrement_countdown();

    struct private_copy_exception
    {};

public:
    explicit test_element(int value);
    test_element(const test_element& other);
    test_element& operator=(const test_element& other) &;
    ~test_element();

    friend bool operator==(const test_element& lhs, const test_element& rhs) noexcept;
    friend bool operator!=(const test_element& lhs, const test_element& rhs) noexcept;
    friend bool operator==(const test_element& element, int value) noexcept;
    friend bool operator!=(const test_element& element, int value) noexcept;

    friend std::ostream& operator<<(std::ostream& ostr, const test_element& e);

    friend void swap(test_element& lhs, test_element& rhs) noexcept;

    friend std::size_t object_count() noexcept;
    friend std::size_t copy_count() noexcept;
    friend void reset_counts() noexcept;
    friend struct private_copy_exception_accessor;
    friend void copy_countdown(std::size_t countdown) noexcept;
};
