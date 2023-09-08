/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file enum_bitmask.hpp
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

#include <initializer_list>

#include "liger/core/types.hpp"

namespace liger {

/**
 * @brief Set the specified bit.
 * 
 * @tparam T   Type of the result integer value
 * @param  bit Index of the bit to be set
 * @return 1 << bit
 */
template <typename T>
constexpr T Bit(uint32 bit);

/**
 * @brief Convenience struct for storing a bit mask of the specified enum class values.
 * 
 * @code{.cpp}
 * enum class Fruit : uint8 {
 *   kApple     = 0x01,
 *   kPineapple = 0x02,
 *   kPeach     = 0x04,
 * };
 * using FruitsMask = EnumBitMask<Fruit, uint8>
 * 
 * // ...
 * 
 * FruitsMask mask{Fruit::kApple, Fruit::kPeach};  // mask = 0b0000'0101
 * mask &= Fruit::kPeach;                          // mask = 0b0000'0100
 * mask |= Fruit::kPineapple;                      // mask = 0b0000'0110
 * @endcode
 * 
 * @tparam EnumT 
 * @tparam UnderlyingT 
 */
template <typename EnumT, typename UnderlyingT = uint32>
struct EnumBitMask {
  UnderlyingT mask{0};

  EnumBitMask() = default;
  EnumBitMask(std::initializer_list<EnumT> enum_values);

  EnumBitMask& operator|=(EnumT enum_value);
  EnumBitMask& operator&=(EnumT enum_value);

  EnumBitMask& operator|=(EnumBitMask other);
  EnumBitMask& operator&=(EnumBitMask other);
};

template <typename EnumT, typename UnderlyingT>
EnumBitMask<EnumT, UnderlyingT> operator|(EnumBitMask<EnumT, UnderlyingT> lhs, EnumBitMask<EnumT, UnderlyingT> rhs);

template <typename EnumT, typename UnderlyingT>
EnumBitMask<EnumT, UnderlyingT> operator&(EnumBitMask<EnumT, UnderlyingT> lhs, EnumBitMask<EnumT, UnderlyingT> rhs);

}  // namespace liger

#define BITMASK_IMPL
#include <liger/core/enum_bitmask.ipp>
#undef BITMASK_IMPL
