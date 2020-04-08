#pragma once

namespace influx {
namespace structures {
template <typename T>
class reference_wrapper {
   public:
    typedef T type;

    inline constexpr reference_wrapper(T& x) noexcept : _element(&x) {}
    inline constexpr reference_wrapper(const reference_wrapper& other) noexcept
        : _element(other._element) {}
    inline constexpr reference_wrapper& operator=(const reference_wrapper& other) noexcept {
        _element = other._element;
        return *this;
    }

    inline constexpr operator T&() const noexcept { return *_element; }
    inline constexpr T& get() const noexcept { return *_element; }
    inline type& operator*() { return *_element; }
    inline type* operator->() { return _element; }

   private:
    type* _element;
};
};  // namespace structures
};  // namespace influx