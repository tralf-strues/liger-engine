/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file EnumBitmask.hpp
 * @date 2023-09-05
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 Nikita Mochalov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <type_traits>

#include <magic_enum_flags.hpp>

#define ENABLE_ENUM_BITMASK(Enum)                  \
  template <>                                      \
  struct liger::EnumEnableBitmask<Enum> {          \
    static constexpr bool kEnabled = true;         \
  };                                               \
  template <>                                      \
  struct magic_enum::customize::enum_range<Enum> { \
    static constexpr bool is_flags = true;         \
  };

namespace liger {

/**
 * @brief Set the specified bit.
 * 
 * @tparam T   Type of the result integer value
 * @param  bit Index of the bit to be set
 * @return 1 << bit
 */
template <typename T = std::uint32_t>
inline constexpr T Bit(std::uint32_t bit) { return 1 << bit; }

template <typename Enum>
struct EnumEnableBitmask {
  static constexpr bool kEnabled = false;
};

}  // namespace liger

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr Enum operator|(Enum lhs, Enum rhs) {
  using UnderlyingType = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr Enum operator&(Enum lhs, Enum rhs) {
  using UnderlyingType = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr Enum operator^(Enum lhs, Enum rhs) {
  using UnderlyingType = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<UnderlyingType>(lhs) ^ static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr Enum operator~(Enum lhs) {
  using UnderlyingType = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(~static_cast<UnderlyingType>(lhs));
}

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr Enum operator|=(Enum& lhs, Enum rhs) {
  using UnderlyingType = typename std::underlying_type<Enum>::type;
  lhs = static_cast<Enum>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
  return lhs;
}

template <typename Enum>
requires liger::EnumEnableBitmask<Enum>::kEnabled
inline constexpr bool EnumBitmaskContains(Enum lhs, Enum rhs) {
  return static_cast<uint64_t>(lhs & rhs) == static_cast<uint64_t>(rhs);
}
