/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DeviceResourceState.hpp
 * @date 2024-01-06
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

#include <Liger-Engine/Core/EnumBitmask.hpp>

namespace liger::rhi {

enum class DeviceResourceState : uint32_t {
  /* Common resource states */
  Undefined          = 0,
  TransferSrc        = Bit(0),
  TransferDst        = Bit(1),

  /* Texture specific states */
  ShaderSampled      = Bit(2),
  ColorTarget        = Bit(3),
  DepthStencilTarget = Bit(4),
  DepthStencilRead   = Bit(5),
  StorageTexture     = Bit(6),
  PresentTexture     = Bit(7),

  /* Buffer specific states */
  VertexBuffer       = Bit(8),
  IndexBuffer        = Bit(9),
  IndirectArgument   = Bit(10),
  UniformBuffer      = Bit(11),
  StorageBuffer      = Bit(12)
};

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::DeviceResourceState);