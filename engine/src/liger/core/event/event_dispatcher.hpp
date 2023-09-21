/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file event_dispatcher.hpp
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

#include "liger/core/event/detail/event_type.hpp"
#include "liger/core/event/event_sink.hpp"

namespace liger {

/**
 * @brief Container of per event type sinks.
 */
class EventDispatcher {
 public:
  /**
   * @brief Get the sink for the particular @ref EventT.
   * 
   * @tparam EventT 
   * @return EventSink<EventT>& 
   */
  template <typename EventT>
  EventSink<EventT>& GetSink();

  /**
   * @brief Dispatch the event to its corresponding @ref EventSink.
   * 
   * @tparam EventT 
   * @param event 
   * @param dispatch_to_all
   * 
   * @return Whether or not the event has been handled.
   */
  template <typename EventT>
  bool Dispatch(const EventT& event, bool dispatch_to_all = false);

 private:
  std::unordered_map<detail::EventTypeId, std::unique_ptr<detail::IBaseEventSink>> sinks_;
};

template <typename EventT>
EventSink<EventT>& EventDispatcher::GetSink() {
  EventSink<EventT>* sink = nullptr;

  detail::EventTypeId type_id = detail::EventTypeIdHolder<EventT>::Value();

  auto it = sinks_.find(type_id);
  if (it == sinks_.end()) {
    it = sinks_.emplace(type_id, std::move(std::make_unique<EventSink<EventT>>())).first;
  }

  return *dynamic_cast<EventSink<EventT>*>(it->second.get());
}

template <typename EventT>
bool EventDispatcher::Dispatch(const EventT& event, bool dispatch_to_all) {
  return GetSink<EventT>().Dispatch(event, dispatch_to_all);
}

}  // namespace liger