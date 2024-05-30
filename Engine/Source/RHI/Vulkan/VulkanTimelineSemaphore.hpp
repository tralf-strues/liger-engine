/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanTimelineSemaphore.hpp
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

#pragma once

#include "VulkanUtils.hpp"

namespace liger::rhi {

class VulkanTimelineSemaphore {
 public:
  VulkanTimelineSemaphore() = default;
  ~VulkanTimelineSemaphore();

  void Init(VkDevice vk_device, uint64_t max_per_frame);
  void Destroy();

  VkSemaphore Get();

  uint64_t GetValue() const;

  uint64_t TimePoint(uint64_t absolute_frame, uint64_t local_time_point) const;

 private:
  VkDevice    vk_device_     {VK_NULL_HANDLE};
  VkSemaphore vk_semaphore_  {VK_NULL_HANDLE};
  uint64_t    max_per_frame_ {0};
};

}  // namespace liger::rhi
