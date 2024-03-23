/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanInstance.hpp
 * @date 2024-02-04
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

#include <Liger-Engine/RHI/Instance.hpp>

#include "VulkanDevice.hpp"

namespace liger::rhi {

class VulkanInstance : public IInstance {
 public:
  ~VulkanInstance() override;

  bool Init(ValidationLevel validation);

  std::span<const IDevice::Info> GetDeviceInfoList() const override;

  std::unique_ptr<IDevice> CreateDevice(uint32_t id, uint32_t frames_in_flight) override;

 private:
  bool FillDeviceInfoList();

  VkInstance      instance_{VK_NULL_HANDLE};
  ValidationLevel validation_{ValidationLevel::None};

  std::vector<VkPhysicalDevice> physical_devices_;
  std::vector<uint32_t>         physical_device_ids_;
  std::vector<IDevice::Info>    device_info_list_;

  VkDebugUtilsMessengerEXT debug_utils_messenger_{VK_NULL_HANDLE};
};

}  // namespace liger::rhi