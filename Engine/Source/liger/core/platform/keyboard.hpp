/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file keyboard.hpp
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
  kRelease = GLFW_RELEASE,
  kPress   = GLFW_PRESS,
  kHold    = GLFW_REPEAT
};

enum class KeyModifier : uint8_t {
  kShift    = GLFW_MOD_SHIFT,
  kControl  = GLFW_MOD_CONTROL,
  kAlt      = GLFW_MOD_ALT,
  kSuper    = GLFW_MOD_SUPER,
  kCapsLock = GLFW_MOD_CAPS_LOCK,
  kNumLock  = GLFW_MOD_NUM_LOCK,
};

using KeyMods = KeyModifier;

enum class Key : uint16_t {
  kA = GLFW_KEY_A,
  kB = GLFW_KEY_B,
  kC = GLFW_KEY_C,
  kD = GLFW_KEY_D,
  kE = GLFW_KEY_E,
  kF = GLFW_KEY_F,
  kG = GLFW_KEY_G,
  kH = GLFW_KEY_H,
  kI = GLFW_KEY_I,
  kJ = GLFW_KEY_J,
  kK = GLFW_KEY_K,
  kL = GLFW_KEY_L,
  kM = GLFW_KEY_M,
  kN = GLFW_KEY_N,
  kO = GLFW_KEY_O,
  kP = GLFW_KEY_P,
  kQ = GLFW_KEY_Q,
  kR = GLFW_KEY_R,
  kS = GLFW_KEY_S,
  kT = GLFW_KEY_T,
  kU = GLFW_KEY_U,
  kV = GLFW_KEY_V,
  kW = GLFW_KEY_W,
  kX = GLFW_KEY_X,
  kY = GLFW_KEY_Y,
  kZ = GLFW_KEY_Z,

  k0 = GLFW_KEY_0,
  k1 = GLFW_KEY_1,
  k2 = GLFW_KEY_2,
  k3 = GLFW_KEY_3,
  k4 = GLFW_KEY_4,
  k5 = GLFW_KEY_5,
  k6 = GLFW_KEY_6,
  k7 = GLFW_KEY_7,
  k8 = GLFW_KEY_8,
  k9 = GLFW_KEY_9,

  kEsc = GLFW_KEY_ESCAPE,
  kTab = GLFW_KEY_TAB,
  kCaps = GLFW_KEY_CAPS_LOCK,
  kLeftShift = GLFW_KEY_LEFT_SHIFT,
  kRightShift = GLFW_KEY_RIGHT_SHIFT,
  kLeftCtrl = GLFW_KEY_LEFT_CONTROL,
  kRightCtrl = GLFW_KEY_RIGHT_CONTROL,
  kLeftAlt = GLFW_KEY_LEFT_ALT,
  kRightAlt = GLFW_KEY_RIGHT_ALT,
  kSpace = GLFW_KEY_SPACE,
  kEnter = GLFW_KEY_ENTER,
};

}  // namespace liger