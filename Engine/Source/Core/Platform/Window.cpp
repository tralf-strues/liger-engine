/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Window.cpp
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

#include <Liger-Engine/Core/Log/Log.hpp>
#include <Liger-Engine/Core/Platform/Window.hpp>

#include <GLFW/glfw3.h>

namespace liger {

Window::Window(uint32_t width, uint32_t height, const std::string_view title) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
  LIGER_ASSERT(window_, "Window", "glfwCreateWindow failed!");
}

Window::~Window() {
  glfwDestroyWindow(window_);
}

GLFWwindow* Window::GetGLFWwindow() { return window_; }

void Window::SetTitle(const std::string_view title) {
  glfwSetWindowTitle(window_, title.data());
}

uint32_t Window::GetWidth() const {
  int32_t width = 0;
  glfwGetWindowSize(window_, &width, nullptr);

  return static_cast<uint32_t>(width);
}

uint32_t Window::GetHeight() const {
  int32_t height = 0;
  glfwGetWindowSize(window_, nullptr, &height);

  return static_cast<uint32_t>(height);
}

uint32_t Window::GetFramebufferWidth() const {
  int32_t width = 0;
  glfwGetFramebufferSize(window_, &width, nullptr);

  return static_cast<uint32_t>(width);
}

uint32_t Window::GetFramebufferHeight() const {
  int32_t height = 0;
  glfwGetFramebufferSize(window_, nullptr, &height);

  return static_cast<uint32_t>(height);
}

void Window::SetOpacity(float opacity) {
  glfwSetWindowOpacity(window_, opacity);
}

float Window::GetOpacity() const {
  return glfwGetWindowOpacity(window_);
}

}  // namespace liger