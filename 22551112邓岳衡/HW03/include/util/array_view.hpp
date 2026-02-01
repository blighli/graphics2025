#pragma once
#include <cstddef>
#include <type_traits>
#include <ranges>
#include <concepts>

// similar to std::span, but could be constructed from single object
template <typename T>
class ArrayView
{
    T *const pArray = nullptr;
    size_t count_ = 0;

public:
    // Default constructor with count 0
    ArrayView() = default;
    // Construct from single object with count 1
    ArrayView(T &data) : pArray(&data), count_(1) {}
    // Construct from top-level array
    template <typename R>
    ArrayView(R &&range)
        requires requires(R r) { 
            requires std::ranges::contiguous_range<R>; 
            requires std::ranges::sized_range<R>;
            requires std::ranges::borrowed_range<R>; 
            requires std::convertible_to<decltype(std::ranges::data(r)), T*>;
        }
        : pArray(std::ranges::data(range)), count_(std::ranges::size(range))
    {
    }

    // Construct from pointer and element count
    ArrayView(T *pData, size_t elementCount) : pArray(pData), count_(elementCount) {}
    // If T has const modifier, compatible construction from corresponding non-const version ArrayView
    ArrayView(const ArrayView<std::remove_const_t<T>> &other) : pArray(other.pointer()), count_(other.count()) {}

    // Getter
    T *pointer() const { return pArray; }
    size_t count() const { return count_; }

    // Const Function
    T &operator[](size_t index) const { return pArray[index]; }
    T *begin() const { return pArray; }
    T *end() const { return pArray + count_; }

    // Non-const Function
    // Disable copy/move assignment (ArrayView aims to simulate "reference to array",
    // used mainly for parameter passing, so keep its underlying address like C++ references, preventing modification after initialization)
    ArrayView &operator=(const ArrayView &) = delete;
};