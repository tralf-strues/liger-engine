/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file rhi.hpp
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

#include <liger/render/rhi/device_info.hpp>

#include <span>

namespace liger::rhi {

class IDevice;

enum class RHI_Family {
  kVulkan,
  kMetal,
  kDirectx12
};

class RHI_Instance {
 public:
  static std::unique_ptr<RHI_Instance> Create(RHI_Family family, bool enable_debug);

 public:
  virtual ~RHI_Instance() = default;

  RHI_Instance(const RHI_Instance& other) = delete;
  RHI_Instance& operator=(const RHI_Instance& other) = delete;

  virtual std::span<const DeviceInfo> GetDeviceInfoList() const = 0;

  virtual std::unique_ptr<IDevice> CreateDevice(uint32_t id) = 0;
};

}  // namespace liger::rhi