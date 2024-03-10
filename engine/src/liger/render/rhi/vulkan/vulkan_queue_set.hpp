/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_queue_set.hpp
 * @date 2024-02-17
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

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <optional>
#include <vector>

namespace liger::rhi {

class VulkanQueueSet {
 public:
   struct QueueFamilyIndices {
    uint32_t main;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
  };

  std::vector<VkDeviceQueueCreateInfo> FillQueueCreateInfos(VkPhysicalDevice physical_device);
  void InitQueues(VkDevice device);

  const QueueFamilyIndices& GetQueueFamilyIndices() const;

  uint32_t GetQueueCount() const;
  VkQueue GetQueueByIdx(uint32_t queue_idx) const;
  uint32_t GetQueueFamilyByIdx(uint32_t queue_idx) const;

  VkQueue GetMainQueue() const;
  std::optional<VkQueue> GetComputeQueue() const;
  std::optional<VkQueue> GetTransferQueue() const;

 private:
  QueueFamilyIndices queue_family_indices_;
  VkQueue            queues_[3];
  uint32_t           queue_count_{0};
};

}  // namespace liger::rhi