/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file types.hpp
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

#pragma once

#include <cstdint>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <vector>

namespace liger {

/* Integer types */
using size_type  = uint64_t;

using int8       = uint8_t;
using int16      = int16_t;
using int32      = int32_t;
using int64      = int64_t;

using uint8      = uint8_t;
using uint16     = uint16_t;
using uint32     = uint32_t;
using uint64     = uint64_t;

/* Strings */
using cstring    = const char*;

}  // namespace liger
