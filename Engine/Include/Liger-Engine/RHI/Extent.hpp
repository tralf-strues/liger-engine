/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Extent.hpp
 * @date 2023-12-07
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

namespace liger::rhi {

struct Extent2D {
  uint32_t x{0};
  uint32_t y{0};

  bool operator==(const Extent2D& rhs) const {
    return (x == rhs.x) && (y == rhs.y);
  }

  Extent2D MipExtent(uint32_t mip) const {
    return Extent2D {.x = (x >> mip), .y = (y >> mip)};
  }
};

struct Extent3D {
  uint32_t x{0};
  uint32_t y{0};
  uint32_t z{0};

  bool operator==(const Extent3D& rhs) const {
    return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
  }
};

}  // namespace liger::rhi