/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Instance.hpp
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

#include <Liger-Engine/RHI/Device.hpp>

#include <span>

namespace liger::rhi {

/**
 * @brief Type of graphics API.
 * 
 * @warning Only @ref GraphicsAPI::Vulkan is supported at the moment!
 */
enum class GraphicsAPI : uint8_t {
  Vulkan,
  D3D12,
  Metal
};

/**
 * @brief The RHI instance over particular GraphicsAPIs. Allows creating devices.
 */
class IInstance {
 public:
  enum class ValidationLevel : uint8_t {
    None,
    DebugInfoOnly,
    Basic,
    Extensive
  };

  IInstance() = default;

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
   * @param id Device identifier, can be obtained via @ref IDevice::Info.
   * @param frames_in_flight The number of frames in flight the device is configured to work with.
   *
   * @return Device or nullptr in case an error occurs.
   */
  virtual std::unique_ptr<IDevice> CreateDevice(uint32_t id, uint32_t frames_in_flight) = 0;

  /**
   * @brief Create a RHI Instance for the particular API.
   *
   * @param api Desired @ref GraphicsAPI.
   * @param validation Desired @ref ValidationLevel.
   *
   * @return RHI instance or nullptr in case an error occurs.
   */
  static std::unique_ptr<IInstance> Create(GraphicsAPI api, ValidationLevel validation);
};

}  // namespace liger::rhi