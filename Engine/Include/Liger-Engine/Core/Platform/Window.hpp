/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Window.hpp
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

#pragma once

#include <string_view>

struct GLFWwindow;

namespace liger {

class PlatformLayer;
class Window;

struct WindowCloseEvent {
  Window* window;
};

class Window {
 public:
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  GLFWwindow* GetGLFWwindow();

  void SetTitle(std::string_view title);

  uint32_t GetWidth() const;
  uint32_t GetHeight() const;

  uint32_t GetFramebufferWidth() const;
  uint32_t GetFramebufferHeight() const;

  void SetOpacity(float opacity);
  float GetOpacity() const;

 private:
  Window(uint32_t width, uint32_t height, std::string_view title);

  GLFWwindow* window_{nullptr};

  friend class PlatformLayer;
};

}  // namespace liger