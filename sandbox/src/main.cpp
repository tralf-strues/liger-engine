/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file main.cpp
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

#include "liger/core/core.hpp"

struct MouseEvent {
  int mouse_key{0};
};

struct MouseEventHandler {
  bool OnMouseEvent(const MouseEvent& event) {
    LIGER_LOG_INFO(liger::LogChannel::kNone, "Mouse event, mouse_key + offset = {0}", event.mouse_key + offset);
    return false;
  }

  int offset{0};
};

int main() {
  liger::int32 i = 0;
  LIGER_LOG_INFO  (liger::LogChannel::kNone,    "Liger Sandbox");
  LIGER_LOG_INFO  (liger::LogChannel::kNone,    "Info none {0}", i++);
  LIGER_LOG_TRACE (liger::LogChannel::kCore,    "Trace core {0}", i++);
  LIGER_LOG_WARN  (liger::LogChannel::kRender,  "Warning render {0}", i++);
  LIGER_LOG_ERROR (liger::LogChannel::kECS,     "Error ecs {0}", i++);
  LIGER_LOG_FATAL (liger::LogChannel::kPhysics, "Fatal physics {0}", i++);

  {
    liger::ScopedTimer timer{liger::LogChannel::kCore, "Sandbox loop"};

    float total = 0.0f;
    for (liger::uint32 i = 0; i < 4096; ++i) {
      total += static_cast<float>(i) / 100.0f;
    }

    total /= 4096.0f;

    LIGER_LOG_INFO(liger::LogChannel::kCore, "total = {0}", total);
  }

  liger::EventDispatcher event_dispatcher;

  MouseEventHandler handler;
  handler.offset = 22;

  event_dispatcher.GetSink<MouseEvent>().Connect<&MouseEventHandler::OnMouseEvent>(handler);
  event_dispatcher.Dispatch<MouseEvent>(MouseEvent{10});

  return 0;
}
