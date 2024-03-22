/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_command_pool.cpp
 * @date 2024-03-02
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

#include <liger/rhi/vulkan/vulkan_command_pool.hpp>
#include <liger/rhi/vulkan/vulkan_utils.hpp>

namespace liger::rhi {

VulkanCommandPool::~VulkanCommandPool() {
  Destroy();
}

void VulkanCommandPool::Init(VkDevice device, uint32_t frames_in_flight, const VulkanQueueSet& queue_set) {
  device_           = device;
  frames_in_flight_ = frames_in_flight;
  queue_count_      = queue_set.GetQueueCount();

  pools_.resize(frames_in_flight_ * queue_count_);

  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    const VkCommandPoolCreateInfo pool_info {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = 0,
      .queueFamilyIndex = queue_set.GetQueueFamilyByIdx(queue_idx)
    };

    for (uint32_t frame_idx = 0; frame_idx < frames_in_flight_; ++frame_idx) {
      VULKAN_CALL(vkCreateCommandPool(device_, &pool_info, nullptr, &GetCommandPool(frame_idx, queue_idx)));
    }
  }
}

void VulkanCommandPool::Destroy() {
  for (auto& pool : pools_) {
    if (pool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(device_, pool, nullptr);
      pool = VK_NULL_HANDLE;
    }
  }

  device_ = VK_NULL_HANDLE;
}

VulkanCommandBuffer VulkanCommandPool::AllocateCommandBuffer(uint32_t frame_idx, uint32_t queue_idx) {
  const VkCommandBufferAllocateInfo allocate_info {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext              = nullptr,
    .commandPool        = GetCommandPool(frame_idx, queue_idx),
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };

  VkCommandBuffer vk_cmds = VK_NULL_HANDLE;
  VULKAN_CALL(vkAllocateCommandBuffers(device_, &allocate_info, &vk_cmds));

  return VulkanCommandBuffer(vk_cmds);
}

void VulkanCommandPool::Reset(uint32_t frame_idx) {
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    VULKAN_CALL(vkResetCommandPool(device_, GetCommandPool(frame_idx, queue_idx), 0));
  }
}

VkCommandPool& VulkanCommandPool::GetCommandPool(uint32_t frame_idx, uint32_t queue_idx) {
  return pools_[frame_idx * queue_count_ + queue_idx];
}

}  // namespace liger::rhi