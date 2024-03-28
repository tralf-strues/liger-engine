/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file TypeMap.hpp
 * @date 2024-03-16
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

#include <Liger-Engine/Core/Log/Log.hpp>
#include <Liger-Engine/Core/LogChannel.hpp>

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace liger::detail {

struct IBaseTypeMapHolder {
 public:
  virtual ~IBaseTypeMapHolder() = default;
};

template <typename T>
struct TypeMapHolder : IBaseTypeMapHolder {
  ~TypeMapHolder() override = default;

  T value;
};

}  // namespace liger::details

namespace liger {

template <template <typename> typename Value>
class TypeMap {
 public:
  template <typename Type>
  Value<Type>& Get() {
    auto it = holders_.find(typeid(Type));
    if (it == holders_.end()) {
      auto result = holders_.emplace(typeid(Type), std::make_unique<detail::TypeMapHolder<Value<Type>>>());
      LIGER_ASSERT(result.second, kLogChannelCore, "Failed to insert TypeMapHolder, Type = {0}", typeid(Type).name());

      it = result.first;
    }

    return static_cast<detail::TypeMapHolder<Value<Type>>*>(it->second.get())->value;
  }

 private:
  std::unordered_map<std::type_index, std::unique_ptr<detail::IBaseTypeMapHolder>> holders_;
};

}  // namespace liger