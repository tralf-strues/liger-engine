/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file callback.hpp
 * @date 2023-09-11
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

#include <liger/core/log/default_log.hpp>

namespace liger {
namespace detail {

template <typename FunctionSignatureT>
class Callback;

template <typename ReturnT, typename... ArgsT>
class Callback<ReturnT(ArgsT...)> {
 private:
  using FunctionSignatureT = ReturnT(ArgsT...);
  using MethodSignatureT   = ReturnT(void*, ArgsT...);

 public:
  template <auto FunctionT>
  void Connect() {
    instance_ = nullptr;
    callable_ = [](void*, ArgsT... args) { return ReturnT(FunctionT(std::forward<ArgsT>(args)...)); };
  }

  template <auto FunctionT, class ClassT>
  void Connect(ClassT& instance) {
    instance_ = &instance;
    callable_ = [](void* instance, ArgsT... args) {
      return ReturnT(((reinterpret_cast<ClassT*>(instance))->*(FunctionT))(std::forward<ArgsT>(args)...));
    };
  }

  ReturnT operator()(ArgsT... args) {
    LIGER_ASSERT(callable_, "Core", "Trying to call nullptr function!");
    return callable_(instance_, args...);
  }

  bool operator==(const Callback<ReturnT(ArgsT...)>& other) {
    return callable_ == other.callable_ && instance_ == other.instance_;
  }

 private:
  void*             instance_{nullptr};
  MethodSignatureT* callable_{nullptr};
};

}  // namespace detail
}  // namespace liger