/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Context.hpp
 * @date 2024-05-11
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

#include <Liger-Engine/RHI/LogChannel.hpp>

#include <any>
#include <typeindex>
#include <unordered_map>

namespace liger::rhi {

class Context {
 public:
  template <typename Data>
  Data& Insert(Data data);

  template <typename Data, typename... Args>
  Data& Emplace(Args&&... args);

  template <typename Data>
  void Remove();

  template <typename Data>
  Data& Get();

  template <typename Data>
  const Data& Get() const;

 private:
  std::unordered_map<std::type_index, std::any> storage_;  // FIXME (tralf-strues): Come up with another way to store
};

template <typename Data>
Data& Context::Insert(Data data) {
  auto& result = (storage_[typeid(Data)] = std::move(data));
  return std::any_cast<Data&>(result);
}

template <typename Data, typename... Args>
Data& Context::Emplace(Args&&... args) {
  return Insert(Data{std::forward<Args>(args)...});
}

template <typename Data>
void Context::Remove() {
  auto it = storage_.find(typeid(Data));
  if (it != storage_.end()) {
    storage_.erase(it);
  }
}

template <typename Data>
Data& Context::Get() {
  auto it = storage_.find(typeid(Data));
  LIGER_ASSERT(it != storage_.end(), kLogChannelRHI, "Trying to access invalid data");

  return std::any_cast<Data&>(it->second);
}

template <typename Data>
const Data& Context::Get() const {
  auto it = storage_.find(typeid(Data));
  LIGER_ASSERT(it != storage_.end(), kLogChannelRHI, "Trying to access invalid data");

  return std::any_cast<const Data&>(it->second);
}

}  // namespace liger::rhi