/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file PlatformLayer.hpp
 * @date 2023-09-19
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

#include <Liger-Engine/Core/Event/EventDispatcher.hpp>
#include <Liger-Engine/Core/Platform/Keyboard.hpp>
#include <Liger-Engine/Core/Platform/Mouse.hpp>
#include <Liger-Engine/Core/Platform/Window.hpp>

namespace liger {

class PlatformLayer {
 public:
  explicit PlatformLayer(EventDispatcher& dispatcher);
  ~PlatformLayer();

  void PollEvents();

  template <typename EventT>
  EventSink<EventT>& GetSink() {
    return dispatcher_.GetSink<EventT>();
  }

  /************************************************************************************************
   * Window
   ************************************************************************************************/
  std::unique_ptr<Window> CreateWindow(uint32_t width, uint32_t height, std::string_view title);

  /************************************************************************************************
   * Input
   ************************************************************************************************/
  bool KeyPressed(Window* window, Key key) const;

  glm::vec2 GetCursorPosition(Window* window) const;
  void SetCursorEnabled(Window* window, bool enabled);

 private:
  void SetupCallbacks(Window* window);

  static void WindowCloseCallback(GLFWwindow* window);

  static void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
  static void ScrollCallback(GLFWwindow* window, double dx, double dy);
  static void MouseMoveCallback(GLFWwindow* window, double x, double y);
  static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods);

  EventDispatcher&                           dispatcher_;
  std::unordered_map<GLFWwindow*, glm::vec2> prev_mouse_pos_;
  std::unordered_map<GLFWwindow*, Window*>   window_wrapper_;
};

}  // namespace liger