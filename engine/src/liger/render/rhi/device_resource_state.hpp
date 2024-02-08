/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file device_resource_state.hpp
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

#include <liger/core/enum_bitmask.hpp>

namespace liger::rhi {

enum class DeviceResourceState : uint32_t {
  /* Common resource states */
  kUndefined          = 0,
  kComputeWrite       = Bit(0),
  kTransferSrc        = Bit(1),
  kTransferDst        = Bit(2),

  /* Texture specific states */
  kShaderSampled      = Bit(3),
  kColorTarget        = Bit(4),
  kDepthStencilTarget = Bit(5),
  kDepthStencilRead   = Bit(6),

  /* Buffer specific states */
  kVertexBuffer       = Bit(7),
  kIndexBuffer        = Bit(8),
  kIndirectArgument   = Bit(9),
  kUniformBuffer      = Bit(10),
  kStorageBuffer      = Bit(11),
};

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::DeviceResourceState);