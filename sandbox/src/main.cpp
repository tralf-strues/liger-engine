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

#include <liger/core/core.hpp>

static bool g_Running{true};

using namespace liger;
using namespace liger::core;

bool OnWindowClose(const WindowCloseEvent& event) {
  LIGER_LOG_INFO(LogChannel::kGameCore, "OnWindowClose: window = {0}", (void*)event.window);
  g_Running = false;
  return true;
}

bool OnMouseScroll(const MouseScrollEvent& event) {
  LIGER_LOG_INFO(LogChannel::kGameCore, "OnMouseScroll: delta = {0}", event.delta);
  return false;
}

bool OnMouseMove(const MouseMoveEvent& event) {
  LIGER_LOG_INFO(LogChannel::kGameCore, "OnMouseMove: pos = {0}, delta = {1}", event.new_position, event.delta);
  return false;
}

bool OnMouseButton(const MouseButtonEvent& event) {
  LIGER_LOG_INFO(LogChannel::kGameCore, "OnMouseButton: button = {0}", event.custom_button_num);
  return false;
}

bool OnKeyEvent(const KeyEvent& event) {
  LIGER_LOG_INFO(LogChannel::kGameCore, "OnKeyEvent: key = {0}", static_cast<int>(event.key));
  return false;
}

int main() {
  int32 i = 0;
  LIGER_LOG_INFO  (LogChannel::kNone,    "Liger Sandbox");
  LIGER_LOG_INFO  (LogChannel::kNone,    "Info none {0}", i++);
  LIGER_LOG_TRACE (LogChannel::kCore,    "Trace core {0}", i++);
  LIGER_LOG_WARN  (LogChannel::kRender,  "Warning render {0}", i++);
  LIGER_LOG_ERROR (LogChannel::kECS,     "Error ecs {0}", i++);
  LIGER_LOG_FATAL (LogChannel::kPhysics, "Fatal physics {0}", i++);

  {
    ScopedTimer timer{LogChannel::kCore, "Sandbox loop"};

    float total = 0.0f;
    for (uint32 i = 0; i < 4096; ++i) {
      total += static_cast<float>(i) / 100.0f;
    }

    total /= 4096.0f;

    LIGER_LOG_INFO(LogChannel::kCore, "total = {0}", total);
  }

  EventDispatcher event_dispatcher;
  PlatformLayer platform_layer{event_dispatcher};

  auto window = std::unique_ptr<Window>(platform_layer.CreateWindow(1280, 720, "Liger Sandbox"));
  window->SetOpacity(0.9f);

  platform_layer.GetSink<WindowCloseEvent>().Connect<&OnWindowClose>();
  platform_layer.GetSink<MouseScrollEvent>().Connect<&OnMouseScroll>();
  platform_layer.GetSink<MouseMoveEvent>().Connect<&OnMouseMove>();
  platform_layer.GetSink<MouseButtonEvent>().Connect<&OnMouseButton>();
  platform_layer.GetSink<KeyEvent>().Connect<&OnKeyEvent>();

  tf::Executor executor;
  tf::Taskflow taskflow;

  tf::Task A = taskflow.emplace([](){ std::cout << "A" << std::endl; }).name("A");  
  tf::Task C = taskflow.emplace([](){ std::cout << "C" << std::endl; }).name("C");  
  tf::Task D = taskflow.emplace([](){ std::cout << "D" << std::endl; }).name("D");  

  tf::Task B = taskflow.emplace([] (tf::Subflow& subflow) {
    std::cout << "Subflow: B" << std::endl;

    tf::Task B1 = subflow.emplace([](){ std::cout << "B1" << std::endl; }).name("B1");  
    tf::Task B2 = subflow.emplace([](){ std::cout << "B2" << std::endl; }).name("B2");  
    tf::Task B3 = subflow.emplace([](){ std::cout << "B3" << std::endl; }).name("B3");  
    B3.succeed(B1, B2);  // B3 runs after B1 and B2
  }).name("B");

  A.precede(B, C);  // A runs before B and C
  D.succeed(B, C);  // D runs after  B and C

  executor.run(taskflow).wait();

  while (g_Running) {
    platform_layer.PollEvents();
  }

  return 0;
}
