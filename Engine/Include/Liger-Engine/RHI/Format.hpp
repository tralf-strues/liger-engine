/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Format.hpp
 * @date 2023-09-26
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

// NOLINTBEGIN
/**
 * @brief Specifies data format
 * 
 * Naming explained:
 * 1. Letters U/S before the type specify whether the type is unsigned/signed
 * 2. Suffix NORM means the type is normalized, i.e in range [0, 1]
 * 3. Suffix SRGB means the type has sRGB nonlinear encoding
 */
enum class Format : uint32_t {
  Invalid = 0,

  /* One-component */
  R8_UNORM,

  R32_UINT,
  R32_SINT,
  R32_SFLOAT,

  D16_UNORM,
  D32_SFLOAT,

  /* Two-component */
  R32G32_UINT,
  R32G32_SINT,
  R32G32_SFLOAT,

  D24_UNORM_S8_UINT,

  /* Three-component */
  R8G8B8_UNORM,
  B8G8R8_UNORM,
  R8G8B8_SRGB,

  B10G11R11_UFLOAT,
  R16G16B16_SFLOAT,
  R32G32B32_SFLOAT,

  /* Four-component */
  R8G8B8A8_UNORM,
  R8G8B8A8_SRGB,
  B8G8R8A8_SRGB,

  R16G16B16A16_SFLOAT,
  R32G32B32A32_SFLOAT
};

/**
 * @param format
 * @return Size of the data format in bytes. 
 */
inline uint32_t GetFormatSize(Format format) {
  switch (format) {
    /* One-component */
    case (Format::R8_UNORM):            { return 1; }
    case (Format::R32_UINT):            { return 4; }
    case (Format::R32_SINT):            { return 4; }
    case (Format::R32_SFLOAT):          { return 4; }

    case (Format::D16_UNORM):           { return 2; }
    case (Format::D32_SFLOAT):          { return 4; }

    /* Two-component */
    case (Format::R32G32_UINT):         { return 8; }
    case (Format::R32G32_SINT):         { return 8; }
    case (Format::R32G32_SFLOAT):       { return 8; }

    case (Format::D24_UNORM_S8_UINT):   { return 4; }

    /* Three-component */
    case (Format::R8G8B8_UNORM):        { return 3; }
    case (Format::B8G8R8_UNORM):        { return 3; }
    case (Format::R8G8B8_SRGB):         { return 3; }

    case (Format::B10G11R11_UFLOAT):    { return 4; }
    case (Format::R16G16B16_SFLOAT):    { return 6; }
    case (Format::R32G32B32_SFLOAT):    { return 12; }

    /* Four-component */
    case (Format::R8G8B8A8_UNORM):      { return 4; }
    case (Format::R8G8B8A8_SRGB):       { return 4; }

    case (Format::R16G16B16A16_SFLOAT): { return 8; }
    case (Format::R32G32B32A32_SFLOAT): { return 16; }

    default: { return 0; }
  }
}
// NOLINTEND

inline constexpr bool IsDepthContainingFormat(Format format) {
  return format == Format::D16_UNORM ||
         format == Format::D32_SFLOAT ||
         format == Format::D24_UNORM_S8_UINT;
}

inline constexpr bool IsDepthStencilFormat(Format format) {
  return format == Format::D24_UNORM_S8_UINT;
}

}  // namespace liger::rhi