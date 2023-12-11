/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file buffer.hpp
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

#include <liger/core/core.hpp>

namespace liger::rhi {

enum class BufferUsageBit : uint32 {
  kTransferSrc   = 0x0000'0001,
  kTransferDst   = 0x0000'0002,
  kUniformBuffer = 0x0000'0010,
  kStorageBuffer = 0x0000'0020,
  kIndexBuffer   = 0x0000'0040,
  kVertexBuffer  = 0x0000'0080,
};

using BufferUsage = EnumBitMask<BufferUsageBit, uint32>;

}  // namespace liger::rhi