#include "test_element.h"

#include <limits>

std::size_t test_element::objects_count_ = 0;
std::size_t test_element::copy_count_ = 0;
std::size_t test_element::throw_copy_countdown_ = std::numeric_limits<std::size_t>::max();

void test_element::decrement_countdown()
{
    if (--throw_copy_countdown_ == 0)
    {
        throw private_copy_exception();
    }
}

test_element::test_element(int value) : value_(value)
{
    objects_count_++;
}

test_element::test_element(const test_element& other) : value_(other.value_)
{
    decrement_countdown();
    objects_count_++;
    copy_count_++;
}

test_element& test_element::operator=(const test_element& other) &
{
    decrement_countdown();
    value_ = other.value_;
    copy_count_++;

    return *this;
}

test_element::~test_element()
{
    objects_count_--;
}

void swap(test_element& lhs, test_element& rhs) noexcept
{
    std::swap(lhs.value_, rhs.value_);
}

bool operator==(const test_element& lhs, const test_element& rhs) noexcept
{
    return lhs.value_ == rhs.value_;
}

bool operator!=(const test_element& lhs, const test_element& rhs) noexcept
{
    return !(lhs == rhs);
}

bool operator==(const test_element& element, int value) noexcept
{
    return element.value_ == value;
}

bool operator!=(const test_element& element, int value) noexcept
{
    return !(element == value);
}

std::ostream& operator<<(std::ostream& ostr, const test_element& e)
{
    return ostr << "test_element(" << e.value_ << ')';
}
