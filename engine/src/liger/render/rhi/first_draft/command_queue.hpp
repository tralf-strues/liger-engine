/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file command_queue.hpp
 * @date 2023-09-27
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
#include <liger/render/rhi/device_resource.hpp>

namespace liger::rhi {

enum class CommandTypes : uint8_t {
  kNone     = 0,
  kGraphics = Bit(0),
  kCompute  = Bit(1),
  kTransfer = Bit(2),
  kPresent  = Bit(3)
};

class CommandQueue final : DeviceResource {
 public:
  struct Info {
    CommandTypes capabilities;
  };

 public:
  explicit CommandQueue(DeviceResource::InternalHandle internal = nullptr);
  ~CommandQueue() override = default;  
};

CommandQueue::CommandQueue(DeviceResource::InternalHandle internal) : DeviceResource(internal) {}

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::CommandTypes);