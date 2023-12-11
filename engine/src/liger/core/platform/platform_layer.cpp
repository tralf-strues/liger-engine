/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file platform_layer.cpp
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

#include <liger/core/platform/platform_layer.hpp>

namespace liger {

PlatformLayer::PlatformLayer(EventDispatcher& dispatcher) : dispatcher_(dispatcher) {
  int32_t result = glfwInit();
  LIGER_ASSERT(result == GLFW_TRUE, "PlatformLayer", "Can't init glfw!");
}

PlatformLayer::~PlatformLayer() {
  glfwTerminate();
}

void PlatformLayer::PollEvents() {
  glfwPollEvents();
}

/************************************************************************************************
 * Window
 ************************************************************************************************/
Window* PlatformLayer::CreateWindow(uint32_t width, uint32_t height, const std::string_view title) {
  auto window = new Window(width, height, title);
  glfwSetWindowUserPointer(window->GetGLFWwindow(), this);

  prev_mouse_pos_[window->GetGLFWwindow()] = glm::vec2{0.0f};
  window_wrapper_[window->GetGLFWwindow()] = window;

  SetupCallbacks(window);

  return window;
}

/************************************************************************************************
 * Callbacks
 ************************************************************************************************/
void PlatformLayer::SetupCallbacks(Window* window) {
  LIGER_ASSERT(window, "PlatformLayer", "Invalid window!");

  /* Window */
  glfwSetWindowCloseCallback(window->GetGLFWwindow(), WindowCloseCallback);

  /* Input */
  glfwSetKeyCallback(window->GetGLFWwindow(), KeyCallback);

  glfwSetCursorPosCallback(window->GetGLFWwindow(), MouseMoveCallback);
  glfwSetMouseButtonCallback(window->GetGLFWwindow(), MouseButtonCallback);
  glfwSetScrollCallback(window->GetGLFWwindow(), ScrollCallback);
}

void PlatformLayer::WindowCloseCallback(GLFWwindow* glfw_window) {
  PlatformLayer* platform = static_cast<PlatformLayer*>(glfwGetWindowUserPointer(glfw_window));
  LIGER_ASSERT(platform, "PlatformLayer", "GLFW window is not associated with a PlatformLayer!");

  WindowCloseEvent event{};
  event.window = platform->window_wrapper_[glfw_window];

  platform->dispatcher_.Dispatch(event);
}

void PlatformLayer::KeyCallback(GLFWwindow* glfw_window, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
  PlatformLayer* platform = static_cast<PlatformLayer*>(glfwGetWindowUserPointer(glfw_window));
  LIGER_ASSERT(platform, "PlatformLayer", "GLFW window is not associated with a PlatformLayer!");

  KeyEvent event{};
  event.key    = static_cast<Key>(key);
  event.action = static_cast<PressAction>(action);
  event.mods   = static_cast<KeyMods>(mods);

  platform->dispatcher_.Dispatch(event);
}

void PlatformLayer::ScrollCallback(GLFWwindow* glfw_window, double dx, double dy) {
  PlatformLayer* platform = static_cast<PlatformLayer*>(glfwGetWindowUserPointer(glfw_window));
  LIGER_ASSERT(platform, "PlatformLayer", "GLFW window is not associated with a PlatformLayer!");

  MouseScrollEvent event{};
  event.delta = glm::vec2{dx, dy};

  platform->dispatcher_.Dispatch(event);
}

void PlatformLayer::MouseMoveCallback(GLFWwindow* glfw_window, double x, double y) {
  PlatformLayer* platform = static_cast<PlatformLayer*>(glfwGetWindowUserPointer(glfw_window));
  LIGER_ASSERT(platform, "PlatformLayer", "GLFW window is not associated with a PlatformLayer!");

  MouseMoveEvent event{};
  event.new_position = glm::vec2{x, y};
  event.delta        = event.new_position - platform->prev_mouse_pos_[glfw_window];

  platform->prev_mouse_pos_[glfw_window] = event.new_position;

  platform->dispatcher_.Dispatch(event);
}

void PlatformLayer::MouseButtonCallback(GLFWwindow* glfw_window, int32_t button, int32_t action, int32_t mods) {
  PlatformLayer* platform = static_cast<PlatformLayer*>(glfwGetWindowUserPointer(glfw_window));
  LIGER_ASSERT(platform, "PlatformLayer", "GLFW window is not associated with a PlatformLayer!");

  bool primary_button = button <= GLFW_MOUSE_BUTTON_MIDDLE;

  MouseButtonEvent event{};
  event.button            = primary_button ? static_cast<MouseButton>(button) : MouseButton::kCustom;
  event.custom_button_num = button;
  event.action            = static_cast<PressAction>(action);
  event.mods              = static_cast<KeyMods>(mods);

  platform->dispatcher_.Dispatch(event);
}

/************************************************************************************************
 * Input
 ************************************************************************************************/
bool PlatformLayer::KeyPressed(Window* window, Key key) const {
  LIGER_ASSERT(window, "PlatformLayer", "Invalid window!");

  auto action = glfwGetKey(window->GetGLFWwindow(), static_cast<int32_t>(key));
  return action == GLFW_PRESS || action == GLFW_REPEAT;
}

glm::vec2 PlatformLayer::GetCursorPosition(Window* window) const {
  LIGER_ASSERT(window, "PlatformLayer", "Invalid window!");

  double x{0.0f};
  double y{0.0f};
  glfwGetCursorPos(window->GetGLFWwindow(), &x, &y);

  return glm::vec2{x, y};
}

void PlatformLayer::SetCursorEnabled(Window* window, bool enabled) {
  LIGER_ASSERT(window, "PlatformLayer", "Invalid window!");
  glfwSetInputMode(window->GetGLFWwindow(), GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

}  // namespace liger