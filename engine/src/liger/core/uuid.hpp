/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file uuid.hpp
 * @date 2023-09-12
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

#include <concepts>
#include <cstdint>
#include <memory>
#include <random>

namespace liger {

template <std::unsigned_integral IntegerType = uint64_t>
class BasicUUID {
 public:
  static constexpr IntegerType kInvalidValue = 0;

  constexpr BasicUUID() = default;
  constexpr explicit BasicUUID(IntegerType value);

  constexpr bool Valid() const;

  constexpr IntegerType Value() const;
  constexpr IntegerType operator*() const;

  static BasicUUID<IntegerType> Generate();

 private:
  IntegerType value_{kInvalidValue};
};

template <std::unsigned_integral IntegerType>
constexpr BasicUUID<IntegerType>::BasicUUID(IntegerType value) : value_(value) {}

template <std::unsigned_integral IntegerType>
constexpr bool BasicUUID<IntegerType>::Valid() const {
  return value_ == kInvalidValue;
}

template <std::unsigned_integral IntegerType>
constexpr IntegerType BasicUUID<IntegerType>::Value() const {
  return value_;
}

template <std::unsigned_integral IntegerType>
constexpr IntegerType BasicUUID<IntegerType>::operator*() const {
  return value_;
}

template <std::unsigned_integral IntegerType>
constexpr bool operator==(BasicUUID<IntegerType> lhs, BasicUUID<IntegerType> rhs) {
  return lhs.Value() == rhs.Value();
}

template <std::unsigned_integral IntegerType>
constexpr bool operator!=(BasicUUID<IntegerType> lhs, BasicUUID<IntegerType> rhs) {
  return lhs.Value() != rhs.Value();
}

template <std::unsigned_integral IntegerType>
BasicUUID<IntegerType> BasicUUID<IntegerType>::Generate() {
  static std::random_device                         random_device;
  static std::mt19937_64                            random_engine{random_device()};
  static std::uniform_int_distribution<IntegerType> uniform_distribution;

  return BasicUUID(uniform_distribution(random_engine));
}

using UUID = BasicUUID<uint64_t>;

}  // namespace liger

namespace std {

template <std::unsigned_integral IntegerType>
struct hash<liger::BasicUUID<IntegerType>> {
  size_t operator()(const liger::BasicUUID<IntegerType>& uuid) const {
    return std::hash<IntegerType>{}(uuid.Value());
  }
};

}  // namespace std