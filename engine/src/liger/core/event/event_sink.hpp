/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file event_sink.hpp
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

#include <liger/core/event/detail/callback.hpp>

namespace liger {

namespace detail {

class IBaseEventSink {
 public:
  virtual ~IBaseEventSink() = default;
};

}  // namespace detail

/**
 * @brief Event sink for callbacks of the specified event.
 * 
 * @tparam EventT Event type.
 */
template <typename EventT>
class EventSink final : public detail::IBaseEventSink {
 private:
  /**
   * @brief Callback type, returns whether or not the event has been handled.
   */
  using CallbackFunctionT = bool(const EventT&);
  
 public:
  ~EventSink() override = default;

  /**
   * @brief Add function callback.
   * 
   * @tparam FunctionT 
   */
  template <auto FunctionT>
  void Connect() {
    detail::Callback<CallbackFunctionT> callback;
    callback.template Connect<FunctionT>();
    callbacks_.emplace_back(std::move(callback));
  }

  /**
   * @brief Add class method callback.
   * 
   * @tparam FunctionT 
   * @tparam ClassT 
   * @param instance 
   */
  template <auto FunctionT, class ClassT>
  void Connect(ClassT& instance) {
    detail::Callback<CallbackFunctionT> callback;
    callback.template Connect<FunctionT, ClassT>(instance);
    callbacks_.emplace_back(std::move(callback));
  }

  /**
   * @brief Remove function callback.
   * 
   * @tparam FunctionT 
   */
  template <auto FunctionT>
  void Remove() {
    detail::Callback<CallbackFunctionT> callback;
    callback.template Connect<FunctionT>();

    for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
      if (*it == callback) {
        callbacks_.erase(it);
        break;
      }
    }
  }

  /**
   * @brief Remove class method callback.
   * 
   * @tparam FunctionT 
   * @tparam ClassT 
   * @param instance 
   */
  template <auto FunctionT, class ClassT>
  void Remove(ClassT& instance) {
    detail::Callback<CallbackFunctionT> callback;
    callback.template Connect<FunctionT, ClassT>(instance);

    for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
      if (*it == callback) {
        callbacks_.erase(it);
        break;
      }
    }
  }

  /**
   * @brief Dispatch the event to all callbacks.
   * 
   * @param event
   * @param dispatch_to_all
   *
   * @return Whether or not the event has been handled.
   */
  bool Dispatch(const EventT& event, bool dispatch_to_all = false) {
    bool event_handled = false;

    for (size_t i = 0; i < callbacks_.size(); ++i) {
      event_handled = event_handled || callbacks_[i](event);

      if (event_handled && !dispatch_to_all) {
        break;
      }
    }

    return event_handled;
  }

 private:
  std::vector<detail::Callback<CallbackFunctionT>> callbacks_;
};

}  // namespace liger