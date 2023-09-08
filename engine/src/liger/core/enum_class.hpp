/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file enum_class.hpp
 * @date 2023-09-07
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

#include "liger/core/types.hpp"

/**
 * @brief Declares an enum class.
 * 
 * Example usage:
 * @code{.cpp}
 * DECLARE_ENUM_CLASS(Fruit, uint32, kApple, kPineapple, kPeach);
 * // ...
 * Fruit fruit = Fruit::kPeach;
 * std::cout << FruitToStr(fruit) << std::endl;  // Prints "Peach"
 * // ...
 * Fruit fruit = StrToFruit("Pineapple");        // fruit = Fruit::kPineapple
 * Fruit invalid_fruit = StrToFruit("Tomato");   // fruit = Fruit::kCount
 * @endcode
 * 
 * @param Type           Type name of the enum class
 * @param UnderlyingType Underlying integer type of the enum class (e.g. uint32)
 * @param ...            List of enum values
 */
#define DECLARE_ENUM_CLASS(Type, UnderlyingType, ...)                                                                 \
  enum class Type : UnderlyingType { __VA_ARGS__, kCount };                                                            \
                                                                                                                       \
  namespace detail {                                                                                                   \
  static char kBaseEnumToString##Type[] = {#__VA_ARGS__};                                                              \
  static cstring kTokensEnumToString##Type[static_cast<UnderlyingType>(Type::kCount)];                                 \
                                                                                                                       \
  static int32 kTmpTokensEnumToString##Type =                                                                          \
      liger::detail::TokenizeEnumString(const_cast<char*>(kBaseEnumToString##Type), sizeof(kBaseEnumToString##Type), \
                                          kTokensEnumToString##Type, static_cast<UnderlyingType>(Type::kCount));       \
  } /* namespace detail */                                                                                             \
                                                                                                                       \
  [[nodiscard]] inline cstring Type##ToStr(Type value) {                                                               \
    return detail::kTokensEnumToString##Type[static_cast<UnderlyingType>(value)];                                      \
  }                                                                                                                    \
                                                                                                                       \
  [[nodiscard]] inline Type StrTo##Type(cstring str) {                                                                 \
    for (UnderlyingType i = 0; i < static_cast<UnderlyingType>(Type::kCount); ++i) {                                   \
      if (std::strcmp(str, detail::kTokensEnumToString##Type[i]) == 0) {                                               \
        return static_cast<Type>(i);                                                                                   \
      }                                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    return Type::kCount;                                                                                               \
  }

namespace liger {
namespace detail {

extern int32 TokenizeEnumString(char* base, int32 length, cstring tokens[], int32 size);

}  // namespace detail
}  // namespace liger