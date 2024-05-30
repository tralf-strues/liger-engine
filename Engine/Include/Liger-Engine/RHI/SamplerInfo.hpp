/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file SamplerInfo.hpp
 * @date 2024-02-03
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

#include <Liger-Engine/RHI/Filter.hpp>

namespace liger::rhi {

struct SamplerInfo {
  enum class AddressMode : uint8_t {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
  };

  enum class BorderColor : uint8_t {
    FloatTransparentBlack,
    IntTransparentBlack,

    FloatOpaqueBlack,
    IntOpaqueBlack,

    FloatOpaqueWhite,
    IntOpaqueWhite
  };

  static constexpr float kMaxLODClampNone = 1000.0f;

  /** Minification filter to apply when sampling. */
  Filter min_filter{Filter::Linear};

  /** Magnification filter to apply when sampling. */
  Filter mag_filter{Filter::Linear};

  /** The addressing mode for U coordinates outside normal range [0,1]. */
  AddressMode address_mode_u{AddressMode::ClampToEdge};

  /** The addressing mode for V coordinates outside normal range [0,1]. */
  AddressMode address_mode_v{AddressMode::ClampToEdge};

  /** The addressing mode for W coordinates outside normal range [0,1]. */
  AddressMode address_mode_w{AddressMode::ClampToEdge};

  /** Specifies border color if any address mode is @ref AddressMode::ClampToBorder. */
  BorderColor border_color{BorderColor::IntOpaqueBlack};

  /**
   * @brief Whether to use anisotropic filtering.
   *
   * @warning Availability of anisotropic filtering can be checked in @ref IDevice::Properties::sampler_anisotropy.
   */
  bool anisotropy_enabled{false};

  /**
   * @brief Anisotropy value clamp used when sampling. Ignored if anisotropic filtering is not enabled.
   *
   * @warning Must be less or equal to @ref IDevice::Properties::max_sampler_anisotropy. 
   */
  float max_anisotropy{0.0f};

  /** Mipmap filter to apply when sampling. */
  Filter mipmap_mode{Filter::Linear};

  /** Used to clamp the minimum of the computed mipmap level. */
  float min_lod{0.0f};

  /**
   * @brief Used to clamp the maximum of the computed mipmap level.
   * 
   * @note If upper clamping is undesirable, then set this parameter to @ref kMaxLODClampNone.
   */
  float max_lod{kMaxLODClampNone};

  /** Offset from the calculated mipmap level. */
  float lod_bias{0.0f};
};

}  // namespace liger::rhi