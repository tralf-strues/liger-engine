/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanQueueSet.cpp
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

#include "VulkanQueueSet.hpp"

#include <Liger-Engine/RHI/LogChannel.hpp>

namespace liger::rhi {

std::vector<VkDeviceQueueCreateInfo> VulkanQueueSet::FillQueueCreateInfos(VkPhysicalDevice physical_device) {
  std::vector<VkDeviceQueueCreateInfo> create_infos;

  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  /* Main queue */
  bool main_queue_found = false;
  for (indices.main = 0; indices.main < queue_family_count; ++indices.main) {
    const auto& properties = queue_families[indices.main];

    if (properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) {
      main_queue_found = true;
      break;
    }
  }

  if (!main_queue_found) {
    LIGER_LOG_ERROR(kLogChannelRHI, "Failed to find a main vulkan queue that supports graphics, compute and transfer!");
    return create_infos;
  }

  /* Compute queue */
  bool compute_queue_found = false;
  for (indices.compute = 0; indices.compute < queue_family_count; ++(*indices.compute)) {
    const auto& properties = queue_families[*indices.compute];

    if ((properties.queueFlags & VK_QUEUE_COMPUTE_BIT) && indices.compute != indices.main) {
      compute_queue_found = true;
      break;
    }
  }

  if (!compute_queue_found) {
    LIGER_LOG_INFO(kLogChannelRHI, "No async compute vulkan queue is found");
    indices.compute = std::nullopt;
  } else {
    LIGER_LOG_INFO(kLogChannelRHI, "Async compute vulkan queue is found!");
  }

  /* Transfer queue */
  bool transfer_queue_found = false;
  for (indices.transfer = 0; indices.transfer < queue_family_count; ++(*indices.transfer)) {
    const auto& properties = queue_families[*indices.transfer];

    bool has_transfer            = (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    bool has_graphics_or_compute = (properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0;

    if (has_transfer && !has_graphics_or_compute) {
      transfer_queue_found = true;
      break;
    }
  }

  if (!transfer_queue_found) {
    LIGER_LOG_INFO(kLogChannelRHI, "No dedicated vulkan queue for transfer is found");
    indices.transfer = std::nullopt;
  } else {
    LIGER_LOG_INFO(kLogChannelRHI, "Dedicated vulkan queue for transfer is found!");
  }

  queue_family_indices_ = indices;

  /* Fill create infos */
  constexpr float kDefaultQueuePriority = 1.0f;

  auto add_queue_create_info = [&](uint32_t family_index) {
    create_infos.emplace_back(VkDeviceQueueCreateInfo {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = 0,
      .queueFamilyIndex = family_index,
      .queueCount       = 1,
      .pQueuePriorities = &kDefaultQueuePriority
    });
  };

  add_queue_create_info(queue_family_indices_.main);
  if (queue_family_indices_.compute.has_value()) {
    add_queue_create_info(*queue_family_indices_.compute);
  }
  if (queue_family_indices_.transfer.has_value()) {
    add_queue_create_info(*queue_family_indices_.transfer);
  }

  return create_infos;
}

void VulkanQueueSet::InitQueues(VkDevice device) {
  queue_count_ = 0;

  vkGetDeviceQueue(device, queue_family_indices_.main, /*queueIndex=*/0, &queues_[queue_count_++]);

  if (queue_family_indices_.compute.has_value()) {
    vkGetDeviceQueue(device, *queue_family_indices_.compute, /*queueIndex=*/0, &queues_[queue_count_++]);
  }

  if (queue_family_indices_.transfer.has_value()) {
    vkGetDeviceQueue(device, *queue_family_indices_.transfer, /*queueIndex=*/0, &queues_[queue_count_++]);
  }
}

const VulkanQueueSet::QueueFamilyIndices& VulkanQueueSet::GetQueueFamilyIndices() const {
  return queue_family_indices_;
}

uint32_t VulkanQueueSet::GetQueueCount() const {
  return queue_count_;
}

VkQueue VulkanQueueSet::GetQueueByIdx(uint32_t queue_idx) const {
  LIGER_ASSERT(queue_idx < queue_count_, kLogChannelRHI, "Trying to access invalid queue!");
  return queues_[queue_idx];
}

uint32_t VulkanQueueSet::GetQueueFamilyByIdx(uint32_t queue_idx) const {
  LIGER_ASSERT(queue_idx < queue_count_, kLogChannelRHI, "Trying to access invalid queue!");

  if (queue_idx == 0) {
    return queue_family_indices_.main;
  }

  if (queue_family_indices_.compute.has_value() && queue_idx == 1) {
    return queue_family_indices_.compute.value();
  }

  return queue_family_indices_.transfer.value();
}

VkQueue VulkanQueueSet::GetMainQueue() const {
  return queues_[0];
}

std::optional<VkQueue> VulkanQueueSet::GetComputeQueue() const {
  return queue_family_indices_.compute.has_value() ? std::optional(queues_[1]) : std::nullopt;
}

std::optional<VkQueue> VulkanQueueSet::GetTransferQueue() const {
  if (!queue_family_indices_.transfer.has_value()) {
    return std::nullopt;
  }

  return queue_family_indices_.compute.has_value() ? std::optional(queues_[2]) : std::optional(queues_[1]);
}

}  // namespace liger::rhi