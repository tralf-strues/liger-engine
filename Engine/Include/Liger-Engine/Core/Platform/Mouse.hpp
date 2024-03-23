/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Mouse.hpp
 * @date 2023-09-17
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

#include <Liger-Engine/Core/Math/Math.hpp>
#include <Liger-Engine/Core/Platform/Keyboard.hpp>

namespace liger {

enum class MouseButton : uint8_t {
  Left,
  Right,
  Middle,
  Custom
};

struct MouseScrollEvent {
  glm::vec2 delta;
};

struct MouseMoveEvent {
  /** @brief New cursor position relative to the top-left corner of the window */
  glm::vec2 new_position;

  /** @brief Delta cursor position */
  glm::vec2 delta;
};

struct MouseButtonEvent {
  /** @brief Mouse button type */
  MouseButton button;

  /** @brief Custom mouse button number in case button = MouseButton::kCustom */
  uint8_t custom_button_num;

  /** @brief Type of mouse button action */
  PressAction action;

  /** @brief Keyboard key modifiers */
  KeyMods mods;
};

}  // namespace liger
