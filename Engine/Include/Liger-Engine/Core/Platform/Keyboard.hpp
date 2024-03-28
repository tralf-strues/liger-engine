/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Keyboard.hpp
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

#include <GLFW/glfw3.h>

namespace liger {

enum class Key : uint16_t;
enum class PressAction : uint8_t;
enum class KeyModifier : uint8_t;
using KeyMods = KeyModifier;

struct KeyEvent {
  Key         key;
  PressAction action;
  KeyMods     mods;
};

enum class PressAction : uint8_t {
  Release = GLFW_RELEASE,
  Press   = GLFW_PRESS,
  Hold    = GLFW_REPEAT
};

enum class KeyModifier : uint8_t {
  Shift    = GLFW_MOD_SHIFT,
  Control  = GLFW_MOD_CONTROL,
  Alt      = GLFW_MOD_ALT,
  Super    = GLFW_MOD_SUPER,
  CapsLock = GLFW_MOD_CAPS_LOCK,
  NumLock  = GLFW_MOD_NUM_LOCK,
};

using KeyMods = KeyModifier;

enum class Key : uint16_t {
  A = GLFW_KEY_A,
  B = GLFW_KEY_B,
  C = GLFW_KEY_C,
  D = GLFW_KEY_D,
  E = GLFW_KEY_E,
  F = GLFW_KEY_F,
  G = GLFW_KEY_G,
  H = GLFW_KEY_H,
  I = GLFW_KEY_I,
  J = GLFW_KEY_J,
  K = GLFW_KEY_K,
  L = GLFW_KEY_L,
  M = GLFW_KEY_M,
  N = GLFW_KEY_N,
  O = GLFW_KEY_O,
  P = GLFW_KEY_P,
  Q = GLFW_KEY_Q,
  R = GLFW_KEY_R,
  S = GLFW_KEY_S,
  T = GLFW_KEY_T,
  U = GLFW_KEY_U,
  V = GLFW_KEY_V,
  W = GLFW_KEY_W,
  X = GLFW_KEY_X,
  Y = GLFW_KEY_Y,
  Z = GLFW_KEY_Z,

  Num0 = GLFW_KEY_0,
  Num1 = GLFW_KEY_1,
  Num2 = GLFW_KEY_2,
  Num3 = GLFW_KEY_3,
  Num4 = GLFW_KEY_4,
  Num5 = GLFW_KEY_5,
  Num6 = GLFW_KEY_6,
  Num7 = GLFW_KEY_7,
  Num8 = GLFW_KEY_8,
  Num9 = GLFW_KEY_9,

  Esc = GLFW_KEY_ESCAPE,
  Tab = GLFW_KEY_TAB,
  Caps = GLFW_KEY_CAPS_LOCK,
  LeftShift = GLFW_KEY_LEFT_SHIFT,
  RightShift = GLFW_KEY_RIGHT_SHIFT,
  LeftCtrl = GLFW_KEY_LEFT_CONTROL,
  RightCtrl = GLFW_KEY_RIGHT_CONTROL,
  LeftAlt = GLFW_KEY_LEFT_ALT,
  RightAlt = GLFW_KEY_RIGHT_ALT,
  Space = GLFW_KEY_SPACE,
  Enter = GLFW_KEY_ENTER,
};

}  // namespace liger