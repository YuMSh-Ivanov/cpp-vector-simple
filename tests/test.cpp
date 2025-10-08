#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <new>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "test_allocator.h"
#include "test_element.h"
#include "vector.h"

std::size_t object_count() noexcept
{
    return test_element::objects_count_;
}

std::size_t copy_count() noexcept
{
    return test_element::copy_count_;
}

void reset_counts() noexcept
{
    test_element::throw_copy_countdown_ = std::numeric_limits<std::size_t>::max();
    test_element::copy_count_ = 0;

    test_allocator::throw_countdown_ = std::numeric_limits<std::size_t>::max();
}

void copy_countdown(std::size_t countdown) noexcept
{
    test_element::throw_copy_countdown_ = countdown;
}

void allocate_countdown(std::size_t countdown) noexcept
{
    test_allocator::throw_countdown_ = countdown;
}

struct private_copy_exception_accessor
{
    using exception = test_element::private_copy_exception;
};
using copy_exception = private_copy_exception_accessor::exception;

template <std::size_t N>
static void require_vector_equals(vector& actual, const int (&expected)[N])
{
    if constexpr (N == 0)
    {
        REQUIRE(actual.empty());
    }
    else
    {
        REQUIRE_FALSE(actual.empty());
    }
    REQUIRE(actual.size() == N);

    if constexpr (N != 0)
    {
        REQUIRE(std::as_const(actual).front() == expected[0]);
        REQUIRE(actual.front() == expected[0]);

        REQUIRE(std::as_const(actual).back() == expected[N - 1]);
        REQUIRE(actual.back() == expected[N - 1]);
    }

    for (std::size_t i = 0; i < N; i++)
    {
        CAPTURE(i);
        REQUIRE(std::as_const(actual)[i] == expected[i]);
        REQUIRE(actual[i] == expected[i]);
    }
}

TEST_CASE("default constructor", "[correctness]")
{
    reset_counts();
    {
        vector v;

        REQUIRE(std::as_const(v).empty());
        REQUIRE(std::as_const(v).size() == 0);
        REQUIRE(std::as_const(v).capacity() == 0);
        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("push_back", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_before_destructor;

    {
        vector v;

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        require_vector_equals(v, {10, 20, 30});
        REQUIRE(v.capacity() >= 3);
        REQUIRE(v.data() != nullptr);
        REQUIRE(std::as_const(v).data() != nullptr);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() >= 3);

        copy_count_before_destructor = copy_count();
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_before_destructor);
}

TEST_CASE("push_back assignment", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        v[0] = test_element(-10);
        v[1] = test_element(-20);
        v[2] = test_element(-30);

        require_vector_equals(v, {-10, -20, -30});

        REQUIRE(object_count() == 3);
    }
    REQUIRE(object_count() == 0);
}

TEST_CASE("reserve", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(5);

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 5);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);

        const test_element* data_after_reserve = std::as_const(v).data();
        REQUIRE(v.data() != nullptr);
        REQUIRE(std::as_const(v).data() != nullptr);

        v.push_back(test_element(1));
        v.push_back(test_element(2));
        v.push_back(test_element(3));
        v.push_back(test_element(4));
        v.push_back(test_element(5));

        require_vector_equals(v, {1, 2, 3, 4, 5});
        REQUIRE(v.capacity() == 5);
        REQUIRE(object_count() == 5);
        REQUIRE(copy_count() == 5);

        REQUIRE(v.data() == data_after_reserve);
        REQUIRE(std::as_const(v).data() == data_after_reserve);

        v.push_back(test_element(10));

        require_vector_equals(v, {1, 2, 3, 4, 5, 10});
        REQUIRE(v.capacity() > 5);
        REQUIRE(object_count() == 6);
        REQUIRE(copy_count() == 11);

        REQUIRE(v.data() != data_after_reserve);
        REQUIRE(std::as_const(v).data() != data_after_reserve);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 11);
}

TEST_CASE("reserve less than capacity", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(5);

        const test_element* data_after_first_reserve = std::as_const(v).data();

        std::size_t reserve_argument = GENERATE(0, 3);
        CAPTURE(reserve_argument);
        v.reserve(reserve_argument);

        REQUIRE(v.capacity() == 5);

        REQUIRE(v.data() == data_after_first_reserve);
        REQUIRE(std::as_const(v).data() == data_after_first_reserve);
    }
}

TEST_CASE("reserve zero", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(0); // nothing should change

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);
        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("push_back from itself", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        REQUIRE(v.capacity() == 3); // Just in case

        auto [index, value] = GENERATE(table<std::size_t, int>({
                {0, 10},
                {1, 20},
                {2, 30}
        }));
        CAPTURE(index, value);

        v.push_back(v[index]);

        require_vector_equals(v, {10, 20, 30, value});

        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 7); // No additional copies
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 7);
}

TEST_CASE("pop_back", "[correctness]")
{
    std::size_t copy_count_after_push_back;
    reset_counts();
    {
        vector v;

        v.push_back(test_element(-1));
        v.push_back(test_element(-2));
        v.push_back(test_element(-3));
        v.push_back(test_element(-4));

        std::size_t capacity_after_push_back = v.capacity();
        copy_count_after_push_back = copy_count();

        v.pop_back();
        require_vector_equals(v, {-1, -2, -3});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);

        v.pop_back();
        require_vector_equals(v, {-1, -2});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(object_count() == 2);
        REQUIRE(copy_count() == copy_count_after_push_back);

        v.pop_back();
        require_vector_equals(v, {-1});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(object_count() == 1);
        REQUIRE(copy_count() == copy_count_after_push_back);

        v.pop_back();
        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("clear empty", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.clear();

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);
        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("clear", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element('x'));
        v.push_back(test_element('y'));
        v.push_back(test_element('z'));

        std::size_t capacity_after_push_back = v.capacity();
        copy_count_after_push_back = copy_count();
        const test_element* data_after_push_back = v.data();

        v.clear();

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("shrink_to_fit after clear", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element(11));
        v.push_back(test_element(22));
        v.push_back(test_element(33));

        copy_count_after_push_back = copy_count();

        v.clear();

        v.shrink_to_fit();

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);

        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("shrink_to_fit after pop_back all elements", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element(11));
        v.push_back(test_element(22));
        v.push_back(test_element(33));

        copy_count_after_push_back = copy_count();

        v.pop_back();
        v.pop_back();
        v.pop_back();

        v.shrink_to_fit();

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);

        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("shrink_to_fit non-empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_at_the_end = static_cast<std::size_t>(-1);
    {
        vector v;

        v.reserve(7);

        v.push_back(test_element('a'));
        v.push_back(test_element('b'));
        v.push_back(test_element('c'));
        v.push_back(test_element('d'));
        v.push_back(test_element('e'));

        std::size_t copy_count_after_push_back = copy_count();

        SECTION("after push_back")
        {
            v.shrink_to_fit();

            require_vector_equals(v, {'a', 'b', 'c', 'd', 'e'});
            REQUIRE(v.capacity() == 5);

            REQUIRE(object_count() == 5);
            REQUIRE(copy_count() == copy_count_after_push_back + 5);

            copy_count_at_the_end = copy_count_after_push_back + 5;
        }

        SECTION("after pop_back")
        {
            v.pop_back();
            v.pop_back();

            v.shrink_to_fit();

            require_vector_equals(v, {'a', 'b', 'c'});
            REQUIRE(v.capacity() == 3);

            REQUIRE(object_count() == 3);
            REQUIRE(copy_count() == copy_count_after_push_back + 3);

            copy_count_at_the_end = copy_count_after_push_back + 3;
        }
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_at_the_end);
}

TEST_CASE("shrink_to_fit when data is full", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(5);

        v.push_back(test_element('a'));
        v.push_back(test_element('b'));
        v.push_back(test_element('c'));
        v.push_back(test_element('d'));
        v.push_back(test_element('e'));

        const test_element* data_after_push_back = v.data();

        v.shrink_to_fit();

        require_vector_equals(v, {'a', 'b', 'c', 'd', 'e'});
        REQUIRE(v.capacity() == 5);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);

        REQUIRE(object_count() == 5);
        REQUIRE(copy_count() == 5);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 5);
}

TEST_CASE("swap empty", "[correctness]")
{
    reset_counts();
    {
        vector v1;
        vector v2;

        swap(v1, v2);

        REQUIRE(v1.empty());
        REQUIRE(v1.size() == 0);
        REQUIRE(v1.capacity() == 0);
        REQUIRE(v1.data() == nullptr);
        REQUIRE(std::as_const(v1).data() == nullptr);

        REQUIRE(v2.empty());
        REQUIRE(v2.size() == 0);
        REQUIRE(v2.capacity() == 0);
        REQUIRE(v2.data() == nullptr);
        REQUIRE(std::as_const(v2).data() == nullptr);

        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("swap empty with non-empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v1;
        vector v2;

        v2.push_back(test_element(-11));
        v2.push_back(test_element(-22));
        v2.push_back(test_element(-33));

        copy_count_after_push_back = copy_count();
        const test_element* data_after_push_back = v2.data();
        std::size_t capacity_after_push_back = v2.capacity();

        swap(v1, v2);

        require_vector_equals(v1, {-11, -22, -33});
        REQUIRE(v1.capacity() == capacity_after_push_back);
        REQUIRE(v1.data() == data_after_push_back);
        REQUIRE(std::as_const(v1).data() == data_after_push_back);

        REQUIRE(v2.empty());
        REQUIRE(v2.size() == 0);
        REQUIRE(v2.capacity() == 0);
        REQUIRE(v2.data() == nullptr);
        REQUIRE(std::as_const(v2).data() == nullptr);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("swap non-empty with empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v1;
        vector v2;

        v1.push_back(test_element(-11));
        v1.push_back(test_element(-22));
        v1.push_back(test_element(-33));

        copy_count_after_push_back = copy_count();
        const test_element* data_after_push_back = v1.data();
        std::size_t capacity_after_push_back = v1.capacity();

        swap(v1, v2);

        REQUIRE(v1.empty());
        REQUIRE(v1.size() == 0);
        REQUIRE(v1.capacity() == 0);
        REQUIRE(v1.data() == nullptr);
        REQUIRE(std::as_const(v1).data() == nullptr);

        require_vector_equals(v2, {-11, -22, -33});
        REQUIRE(v2.capacity() == capacity_after_push_back);
        REQUIRE(v2.data() == data_after_push_back);
        REQUIRE(std::as_const(v2).data() == data_after_push_back);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("swap non-empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v1;
        vector v2;

        v1.push_back(test_element(111));
        v1.push_back(test_element(222));
        v1.push_back(test_element(333));

        v2.push_back(test_element(1));
        v2.push_back(test_element(2));
        v2.push_back(test_element(3));
        v2.push_back(test_element(4));
        v2.push_back(test_element(5));

        copy_count_after_push_back = copy_count();
        const test_element* lhs_data_after_push_back = v1.data();
        std::size_t lhs_capacity_after_push_back = v1.capacity();
        const test_element* rhs_data_after_push_back = v2.data();
        std::size_t rhs_capacity_after_push_back = v2.capacity();

        swap(v1, v2);

        require_vector_equals(v1, {1, 2, 3, 4, 5});
        REQUIRE(v1.capacity() == rhs_capacity_after_push_back);
        REQUIRE(v1.data() == rhs_data_after_push_back);
        REQUIRE(std::as_const(v1).data() == rhs_data_after_push_back);

        require_vector_equals(v2, {111, 222, 333});
        REQUIRE(v2.capacity() == lhs_capacity_after_push_back);
        REQUIRE(v2.data() == lhs_data_after_push_back);
        REQUIRE(std::as_const(v2).data() == lhs_data_after_push_back);

        REQUIRE(object_count() == 8);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("swap empty with itself", "[correctness]")
{
    reset_counts();
    {
        vector v;

        swap(v, v);

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);
        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }
    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("swap non-empty with itself", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element(100));
        v.push_back(test_element(200));
        v.push_back(test_element(300));

        copy_count_after_push_back = copy_count();
        const test_element* data_after_push_back = v.data();
        std::size_t capacity_after_push_back = v.capacity();

        swap(v, v);

        require_vector_equals(v, {100, 200, 300});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }
    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("copy constructor from empty", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(2);

        vector copy(std::as_const(v));

        REQUIRE(copy.empty());
        REQUIRE(copy.size() == 0);
        REQUIRE(copy.capacity() == 0);
        REQUIRE(copy.data() == nullptr);
        REQUIRE(std::as_const(copy).data() == nullptr);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("copy constructor from non-empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element(-11));
        v.push_back(test_element(-22));
        v.push_back(test_element(-33));
        v.push_back(test_element(-44));
        v.push_back(test_element(-55));

        copy_count_after_push_back = copy_count();

        vector copy(std::as_const(v));

        require_vector_equals(copy, {-11, -22, -33, -44, -55});
        REQUIRE(copy.capacity() == 5);
        REQUIRE(object_count() == 10);
        REQUIRE(copy_count() == copy_count_after_push_back + 5);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back + 5);
}

TEST_CASE("copy assign empty to empty", "[correctness]")
{
    reset_counts();
    {
        vector from;
        vector to;

        vector& assign_result = to = std::as_const(from);
        REQUIRE(&assign_result == &to);

        REQUIRE(to.empty());
        REQUIRE(to.size() == 0);
        REQUIRE(to.capacity() == 0);
        REQUIRE(to.data() == nullptr);
        REQUIRE(std::as_const(to).data() == nullptr);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("copy assign empty to non-empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector from;
        vector to;

        to.push_back(test_element(1));
        to.push_back(test_element(2));
        to.push_back(test_element(3));

        copy_count_after_push_back = copy_count();
        std::size_t capacity_after_push_back = to.capacity();

        vector& assign_result = to = std::as_const(from);
        REQUIRE(&assign_result == &to);

        REQUIRE(to.empty());
        REQUIRE(to.size() == 0);
        REQUIRE(to.capacity() <= capacity_after_push_back);
        if (to.capacity() == 0)
        {
            REQUIRE(to.data() == nullptr);
            REQUIRE(std::as_const(to).data() == nullptr);
        }
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("copy assign non-empty to empty", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector from;
        vector to;

        from.push_back(test_element(-1));
        from.push_back(test_element(-2));
        from.push_back(test_element(-3));
        from.push_back(test_element(-4));
        from.push_back(test_element(-5));

        copy_count_after_push_back = copy_count();

        vector& assign_result = to = std::as_const(from);
        REQUIRE(&assign_result == &to);

        require_vector_equals(to, {-1, -2, -3, -4, -5});
        REQUIRE(to.capacity() <= 5);
        REQUIRE(object_count() == 10);
        REQUIRE(copy_count() == copy_count_after_push_back + 5);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back + 5);
}

TEST_CASE("copy assign large to small", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector from;
        vector to;

        from.push_back(test_element(-1));
        from.push_back(test_element(-2));
        from.push_back(test_element(-3));
        from.push_back(test_element(-4));
        from.push_back(test_element(-5));

        to.push_back(test_element(1));
        to.push_back(test_element(2));
        to.push_back(test_element(3));

        copy_count_after_push_back = copy_count();

        vector& assign_result = to = std::as_const(from);
        REQUIRE(&assign_result == &to);

        require_vector_equals(to, {-1, -2, -3, -4, -5});
        REQUIRE(to.capacity() <= from.capacity());
        REQUIRE(object_count() == 10);
        REQUIRE(copy_count() == copy_count_after_push_back + 5);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back + 5);
}

TEST_CASE("copy assign small to large", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector from;
        vector to;

        from.push_back(test_element(-1));
        from.push_back(test_element(-2));
        from.push_back(test_element(-3));

        to.push_back(test_element(1));
        to.push_back(test_element(2));
        to.push_back(test_element(3));
        to.push_back(test_element(4));
        to.push_back(test_element(5));

        copy_count_after_push_back = copy_count();
        std::size_t capacity_after_push_back = to.capacity();

        vector& assign_result = to = std::as_const(from);
        REQUIRE(&assign_result == &to);

        require_vector_equals(to, {-1, -2, -3});
        REQUIRE(to.capacity() <= capacity_after_push_back);
        REQUIRE(object_count() == 6);
        REQUIRE(copy_count() == copy_count_after_push_back + 3);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back + 3);
}

TEST_CASE("copy assign empty from itself", "[correctness]")
{
    reset_counts();
    {
        vector v;

        vector& assign_result = v = v;
        REQUIRE(&assign_result == &v);

        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 0);
        REQUIRE(v.data() == nullptr);
        REQUIRE(std::as_const(v).data() == nullptr);
        REQUIRE(object_count() == 0);
        REQUIRE(copy_count() == 0);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 0);
}

TEST_CASE("copy assign non-empty from itself", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element('u'));
        v.push_back(test_element('v'));
        v.push_back(test_element('w'));

        copy_count_after_push_back = copy_count();
        std::size_t capacity_after_push_back = v.capacity();
        const test_element* data_after_push_back = v.data();

        vector& assign_result = v = v;
        REQUIRE(&assign_result == &v);

        require_vector_equals(v, {'u', 'v', 'w'});

        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("insert without reallocation", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(4);

        v.push_back(test_element(1));
        v.push_back(test_element(2));
        v.push_back(test_element(3));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 4));
        CAPTURE(insert_index);

        test_element* insert_result = v.insert(v.data() + insert_index, test_element(100));
        REQUIRE(insert_result == v.data() + insert_index);

        int expected[4];
        for (std::size_t i = 0; i < std::size(expected); i++)
        {
            if (i < insert_index)
            {
                expected[i] = static_cast<int>(i) + 1;
            }
            else if (i == insert_index)
            {
                expected[i] = 100;
            }
            else
            {
                expected[i] = static_cast<int>(i);
            }
        }
        require_vector_equals(v, expected);

        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 4);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 4);
}

TEST_CASE("insert with reallocation", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(1));
        v.push_back(test_element(2));
        v.push_back(test_element(3));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 4));
        CAPTURE(insert_index);

        test_element* insert_result = v.insert(v.data() + insert_index, test_element(100));
        REQUIRE(insert_result == v.data() + insert_index);

        int expected[4];
        for (std::size_t i = 0; i < std::size(expected); i++)
        {
            if (i < insert_index)
            {
                expected[i] = static_cast<int>(i) + 1;
            }
            else if (i == insert_index)
            {
                expected[i] = 100;
            }
            else
            {
                expected[i] = static_cast<int>(i);
            }
        }
        require_vector_equals(v, expected);

        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 7);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 7);
}

TEST_CASE("insert to itself", "[correctness]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 4));
        std::size_t element_index = GENERATE(range<std::size_t>(0, 3));
        CAPTURE(insert_index, element_index);

        test_element* insert_result = v.insert(v.data() + insert_index, v[element_index]);
        REQUIRE(insert_result == v.data() + insert_index);

        int expected[4];
        for (std::size_t i = 0; i < std::size(expected); i++)
        {
            if (i < insert_index)
            {
                expected[i] = static_cast<int>(i) * 10 + 10;
            }
            else if (i == insert_index)
            {
                expected[i] = static_cast<int>(element_index) * 10 + 10;
            }
            else
            {
                expected[i] = static_cast<int>(i) * 10;
            }
        }
        require_vector_equals(v, expected);

        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 7);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 7);
}

TEST_CASE("erase", "[correctness]")
{
    reset_counts();
    std::size_t copy_count_after_push_back;
    {
        vector v;

        v.push_back(test_element(-10));
        v.push_back(test_element(-20));
        v.push_back(test_element(-30));
        v.push_back(test_element(-40));

        copy_count_after_push_back = copy_count();

        std::size_t erase_index = GENERATE(range<std::size_t>(0, 4));
        CAPTURE(erase_index);

        test_element* erase_result = v.erase(v.data() + erase_index);
        REQUIRE(erase_result == v.data() + erase_index);

        int expected[3];
        for (std::size_t i = 0; i < std::size(expected); i++)
        {
            if (i < erase_index)
            {
                expected[i] = static_cast<int>(i) * -10 - 10;
            }
            else
            {
                expected[i] = static_cast<int>(i) * -10 - 20;
            }
        }
        require_vector_equals(v, expected);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("noexcept methods", "[exception safety]")
{
    vector v;
    REQUIRE(noexcept(vector())); // default constructor is noexcept
    REQUIRE(noexcept(v.~vector())); // destructor is noexcept

    REQUIRE(noexcept(std::as_const(v).front())); // front for const is noexcept
    REQUIRE(noexcept(v.front())); // front for non-const is noexcept
    REQUIRE(noexcept(std::as_const(v).back())); // back for const is noexcept
    REQUIRE(noexcept(v.back())); // back for non-const is noexcept
    REQUIRE(noexcept(std::as_const(v)[0])); // operator[] for const is noexcept
    REQUIRE(noexcept(v[0])); // operator[] for non-const is noexcept

    REQUIRE(noexcept(v.empty())); // empty is noexcept
    REQUIRE(noexcept(v.size())); // size is noexcept
    REQUIRE(noexcept(v.capacity())); // size is noexcept
    REQUIRE(noexcept(std::as_const(v).data())); // data for const is noexcept
    REQUIRE(noexcept(v.data())); // data for non-const is noexcept

    REQUIRE(noexcept(v.clear())); // clear is noexcept
    REQUIRE(noexcept(v.pop_back())); // pop_back is noexcept
    REQUIRE(noexcept(v.erase(nullptr))); // erase is noexcept

    REQUIRE(noexcept(swap(v, v))); // swap is noexcept
}

TEST_CASE("push_back guarantee (no reallocation)", "[exception safety]")
{
    reset_counts();

    {
        vector v;

        v.reserve(4);

        const test_element* data_after_reserve = v.data();

        copy_countdown(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));

        REQUIRE_THROWS_AS(v.push_back(test_element(30)), copy_exception);

        require_vector_equals(v, {10, 20});
        REQUIRE(v.capacity() == 4);
        REQUIRE(v.data() == data_after_reserve);
        REQUIRE(std::as_const(v).data() == data_after_reserve);
        REQUIRE(object_count() == 2);
        REQUIRE(copy_count() == 2);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 2);
}

TEST_CASE("push_back guarantee (reallocation, copy throws)", "[exception safety]")
{
    reset_counts();

    {
        vector v;

        v.reserve(2);

        const test_element* data_after_reserve = v.data();

        copy_countdown(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));

        REQUIRE_THROWS_AS(v.push_back(test_element(30)), copy_exception);

        require_vector_equals(v, {10, 20});
        REQUIRE(v.capacity() == 2);
        REQUIRE(v.data() == data_after_reserve);
        REQUIRE(std::as_const(v).data() == data_after_reserve);
        REQUIRE(object_count() == 2);
        REQUIRE(copy_count() == 2);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 2);
}

TEST_CASE("push_back guarantee (reallocation, allocation throws)", "[exception safety]")
{
    reset_counts();

    {
        vector v;

        v.reserve(2);

        const test_element* data_after_reserve = v.data();

        allocate_countdown(1);

        v.push_back(test_element(10));
        v.push_back(test_element(20));

        REQUIRE_THROWS_AS(v.push_back(test_element(30)), std::bad_alloc);

        require_vector_equals(v, {10, 20});
        REQUIRE(v.capacity() == 2);
        REQUIRE(v.data() == data_after_reserve);
        REQUIRE(std::as_const(v).data() == data_after_reserve);
        REQUIRE(object_count() == 2);
        REQUIRE(copy_count() == 2);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 2);
}

TEST_CASE("reserve guarantee (copy throws)", "[exception safety]")
{
    reset_counts();

    std::size_t copy_count_after_push_back;

    {
        vector v;

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));
        v.push_back(test_element(40));

        copy_count_after_push_back = copy_count();
        std::size_t capacity_after_push_back = v.capacity();
        const test_element* data_after_push_back = v.data();

        copy_countdown(3);

        REQUIRE_THROWS_AS(v.reserve(capacity_after_push_back + 10), copy_exception);

        require_vector_equals(v, {10, 20, 30, 40});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == copy_count_after_push_back + 2); // two are successfully copied
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back + 2);
}

TEST_CASE("reserve guarantee (allocation throws)", "[exception safety]")
{
    reset_counts();

    std::size_t copy_count_after_push_back;

    {
        vector v;

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));
        v.push_back(test_element(40));

        copy_count_after_push_back = copy_count();
        std::size_t capacity_after_push_back = v.capacity();
        const test_element* data_after_push_back = v.data();

        allocate_countdown(1);

        REQUIRE_THROWS_AS(v.reserve(capacity_after_push_back + 10), std::bad_alloc);

        require_vector_equals(v, {10, 20, 30, 40});
        REQUIRE(v.capacity() == capacity_after_push_back);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == copy_count_after_push_back);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == copy_count_after_push_back);
}

TEST_CASE("shrink_to_fit guarantee (copy throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(10);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));
        v.push_back(test_element(40));

        const test_element* data_after_push_back = v.data();

        copy_countdown(2);

        REQUIRE_THROWS_AS(v.shrink_to_fit(), copy_exception);

        require_vector_equals(v, {10, 20, 30, 40});
        REQUIRE(v.capacity() == 10);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 4 + 1); // one is successfully copied
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 4 + 1);
}

TEST_CASE("shrink_to_fit guarantee (allocation throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(10);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));
        v.push_back(test_element(40));

        const test_element* data_after_push_back = v.data();

        allocate_countdown(1);

        REQUIRE_THROWS_AS(v.shrink_to_fit(), std::bad_alloc);

        require_vector_equals(v, {10, 20, 30, 40});
        REQUIRE(v.capacity() == 10);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 4);
        REQUIRE(copy_count() == 4);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 4);
}

TEST_CASE("insert guarantee (no reallocation)", "[exception safety]")
{
    reset_counts();

    {
        vector v;

        v.reserve(5);

        copy_countdown(4);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 3));
        CAPTURE(insert_index);

        const test_element* data_after_push_back = v.data();

        REQUIRE_THROWS_AS(v.insert(v.data() + insert_index, test_element(40)), copy_exception);

        require_vector_equals(v, {10, 20, 30});
        REQUIRE(v.capacity() == 5);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == 3);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 3);
}

TEST_CASE("insert guarantee (reallocation, copy throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 3));
        std::size_t failed_copy_number = GENERATE(range<std::size_t>(1, 4));
        CAPTURE(insert_index, failed_copy_number);

        copy_countdown(failed_copy_number);

        const test_element* data_after_push_back = v.data();

        REQUIRE_THROWS_AS(v.insert(v.data() + insert_index, test_element(40)), copy_exception);

        require_vector_equals(v, {10, 20, 30});
        REQUIRE(v.capacity() == 3);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == 3 + (failed_copy_number - 1));
    }

    REQUIRE(object_count() == 0);
}

TEST_CASE("insert guarantee (reallocation, allocation throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        std::size_t insert_index = GENERATE(range<std::size_t>(0, 3));
        CAPTURE(insert_index);

        const test_element* data_after_push_back = v.data();

        allocate_countdown(1);

        REQUIRE_THROWS_AS(v.insert(v.data() + insert_index, test_element(40)), std::bad_alloc);

        require_vector_equals(v, {10, 20, 30});
        REQUIRE(v.capacity() == 3);
        REQUIRE(v.data() == data_after_push_back);
        REQUIRE(std::as_const(v).data() == data_after_push_back);
        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == 3);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 3);
}

TEST_CASE("copy constructor guarantee (copy throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        copy_countdown(3);

        REQUIRE_THROWS_AS(vector(v), copy_exception);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == 3 + 2); // two are successfully copied
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 3 + 2);
}

TEST_CASE("copy constructor guarantee (allocation throws)", "[exception safety]")
{
    reset_counts();
    {
        vector v;

        v.reserve(3);

        v.push_back(test_element(10));
        v.push_back(test_element(20));
        v.push_back(test_element(30));

        allocate_countdown(1);

        REQUIRE_THROWS_AS(vector(v), std::bad_alloc);

        REQUIRE(object_count() == 3);
        REQUIRE(copy_count() == 3);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 3);
}

TEST_CASE("copy assignment guarantee (copy throws)", "[exception safety]")
{
    reset_counts();
    {
        vector to;
        vector from;

        to.reserve(4);
        to.push_back(test_element(-10));
        to.push_back(test_element(-20));
        to.push_back(test_element(-30));

        const test_element* to_data_after_push_back = to.data();

        from.reserve(15);
        from.push_back(test_element(-1));
        from.push_back(test_element(-2));
        from.push_back(test_element(-3));
        from.push_back(test_element(-4));
        from.push_back(test_element(-5));
        from.push_back(test_element(-6));
        from.push_back(test_element(-7));

        copy_countdown(5);

        REQUIRE_THROWS_AS(to = from, copy_exception);

        require_vector_equals(to, {-10, -20, -30});
        REQUIRE(to.capacity() == 4);
        REQUIRE(to.data() == to_data_after_push_back);
        REQUIRE(std::as_const(to).data() == to_data_after_push_back);
        REQUIRE(object_count() == 10);
        REQUIRE(copy_count() == 10 + 4); // four are successfully copied
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 10 + 4);
}

TEST_CASE("copy assignment guarantee (allocation throws)", "[exception safety]")
{
    reset_counts();
    {
        vector to;
        vector from;

        to.reserve(4);
        to.push_back(test_element(-10));
        to.push_back(test_element(-20));
        to.push_back(test_element(-30));

        const test_element* to_data_after_push_back = to.data();

        from.reserve(15);
        from.push_back(test_element(-1));
        from.push_back(test_element(-2));
        from.push_back(test_element(-3));
        from.push_back(test_element(-4));
        from.push_back(test_element(-5));
        from.push_back(test_element(-6));
        from.push_back(test_element(-7));

        allocate_countdown(1);

        REQUIRE_THROWS_AS(to = from, std::bad_alloc);

        require_vector_equals(to, {-10, -20, -30});
        REQUIRE(to.capacity() == 4);
        REQUIRE(to.data() == to_data_after_push_back);
        REQUIRE(std::as_const(to).data() == to_data_after_push_back);
        REQUIRE(object_count() == 10);
        REQUIRE(copy_count() == 10);
    }

    REQUIRE(object_count() == 0);
    REQUIRE(copy_count() == 10);
}

TEST_CASE("small size capacity", "[performance]")
{
    reset_counts();

    vector v;

    v.push_back(test_element(0));

    // It's too large for a vector of one element
    REQUIRE(v.capacity() < 50);
}

TEST_CASE("large size capacity", "[performance]")
{
    reset_counts();

    vector v;

    for (std::size_t i = 0; i < 100; i++)
    {
        v.push_back(test_element(static_cast<int>(i)));
    }

    // It's too large for a vector of 100 elements
    REQUIRE(v.capacity() < 50'000);
}

TEST_CASE("push_back performance", "[performance]")
{
    reset_counts();

    vector v;

    const std::size_t SIZE = 10'000;

    for (std::size_t i = 0; i < SIZE; i++)
    {
        v.push_back(test_element(static_cast<int>(i) * 7));
    }

    // any sensible amortized constant push_back should be ok
    REQUIRE(copy_count() <= SIZE * static_cast<std::size_t>(std::sqrt(SIZE)));
}

TEST_CASE("insert near the end performance", "[performance]")
{
    reset_counts();

    vector v;

    const std::size_t SIZE = 10'000;

    v.push_back(test_element(0));
    v.push_back(test_element(3));
    for (std::size_t i = 2; i < SIZE; i++)
    {
        // two elements after the insertable one
        v.insert(v.data() + i - 2, test_element(static_cast<int>(i) * 3));
    }

    // any sensible amortized constant insert should be ok
    REQUIRE(copy_count() <= SIZE * (static_cast<std::size_t>(std::sqrt(SIZE)) + 2));
}
