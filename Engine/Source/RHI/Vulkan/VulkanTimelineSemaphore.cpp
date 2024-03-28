/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanTimelineSemaphore.cpp
 * @date 2024-02-25
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

#include "VulkanTimelineSemaphore.hpp"

namespace liger::rhi {

VulkanTimelineSemaphore::~VulkanTimelineSemaphore() {
  Destroy();
}

void VulkanTimelineSemaphore::Init(VkDevice vk_device, uint64_t max_per_frame) {
  vk_device_     = vk_device;
  max_per_frame_ = max_per_frame;

  const VkSemaphoreTypeCreateInfo semaphore_type_info {
    .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    .pNext         = nullptr,
    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    .initialValue  = 0
  };

  const VkSemaphoreCreateInfo create_info {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = &semaphore_type_info,
    .flags = 0
  };

  VULKAN_CALL(vkCreateSemaphore(vk_device, &create_info, nullptr, &vk_semaphore_));
}

void VulkanTimelineSemaphore::Destroy() {
  if (vk_semaphore_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(vk_device_, vk_semaphore_, nullptr);
    vk_semaphore_ = VK_NULL_HANDLE;
  }

  vk_device_     = VK_NULL_HANDLE;
  max_per_frame_ = 0;
}

VkSemaphore VulkanTimelineSemaphore::Get() {
  return vk_semaphore_;
}

uint64_t VulkanTimelineSemaphore::TimePoint(uint64_t absolute_frame, uint64_t local_time_point) const {
  return absolute_frame * max_per_frame_ + local_time_point;
}

}  // namespace liger::rhi