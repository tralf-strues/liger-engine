/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file rhi_instance.hpp
 * @date 2023-12-10
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

#include <liger/render/rhi/device.hpp>

#include <span>

namespace liger::rhi {

/**
 * @brief Type of graphics API.
 * 
 * @warning Only GraphicsAPI::kVulkan is supported at the moment!
 */
enum class GraphicsAPI : uint8_t {
  kVulkan,
  kD3D12,
  kMetal
};

/**
 * @brief The RHI instance over particular GraphicsAPIs. Allows creating devices.
 */
class IInstance {
 public:
  IInstance(const IInstance& other) = delete;
  IInstance& operator=(const IInstance& other) = delete;

  virtual ~IInstance() = default;

  /**
   * @brief Get the available devices' info list.
   */
  virtual std::span<const IDevice::Info> GetDeviceInfoList() const = 0;

  /**
   * @brief Create device, based on the physical device type.
   * 
   * @param id Device identifier, can be obtained via IDevice::Info
   */
  virtual std::unique_ptr<IDevice> CreateDevice(uint32_t id) = 0;

  /**
   * @brief Create a RHI Instance for the particular API.
   * 
   * @param api Desired GraphicsAPI
   * @param enable_debug Whether to enable debug information, validation, etc. 
   */
  static std::unique_ptr<IInstance> Create(GraphicsAPI api, bool enable_debug);
};

}  // namespace liger::rhi