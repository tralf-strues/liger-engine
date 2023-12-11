/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file device_resource.hpp
 * @date 2023-12-11
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

#include <memory>

namespace liger::rhi {

class IResource {
 public:
  enum class State : uint32_t {
    /* Common resource states */
    kUndefined    = 0,
    kGraphicsRead = Bit(0),
    kComputeRead  = Bit(1),
    kComputeWrite = Bit(2),
    kTransferSrc  = Bit(3),
    kTransferDst  = Bit(4),

    /* Texture specific states */
    kColorTarget        = Bit(5),
    kDepthStencilTarget = Bit(6),
    kDepthStencilRead   = Bit(7),

    /* Buffer specific states */
    kVertexBuffer     = Bit(8),
    kIndexBuffer      = Bit(9),
    kIndirectArgument = Bit(10),
  };

  virtual ~IResource() = default;
};

using ResourceHandle = std::shared_ptr<IResource>;

constexpr ResourceHandle kNullHandle = nullptr;

}  // namespace liger::rhi