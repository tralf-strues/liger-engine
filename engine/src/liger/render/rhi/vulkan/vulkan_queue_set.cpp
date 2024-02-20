/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_queue_set.cpp
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

#include <liger/core/log/default_log.hpp>
#include <liger/render/rhi/rhi_log_channel.hpp>
#include <liger/render/rhi/vulkan/vulkan_queue_set.hpp>

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

    if (properties.queueFlags == VK_QUEUE_TRANSFER_BIT) {
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
  vkGetDeviceQueue(device, queue_family_indices_.main, /*queueIndex=*/0, &main_queue_);

  if (queue_family_indices_.compute.has_value()) {
    vkGetDeviceQueue(device, *queue_family_indices_.compute, /*queueIndex=*/0, &compute_queue_);
  }

  if (queue_family_indices_.transfer.has_value()) {
    vkGetDeviceQueue(device, *queue_family_indices_.transfer, /*queueIndex=*/0, &transfer_queue_);
  }
}

uint32_t VulkanQueueSet::GetQueueCount() const {
  uint32_t queue_count = 1;

  if (GetComputeQueue()) { ++queue_count; }
  if (GetTransferQueue()) { ++queue_count; }

  return queue_count;
}

const VulkanQueueSet::QueueFamilyIndices& VulkanQueueSet::GetQueueFamilyIndices() const {
  return queue_family_indices_;
}

VkQueue VulkanQueueSet::GetMainQueue() const {
  return main_queue_;
}

std::optional<VkQueue> VulkanQueueSet::GetComputeQueue() const {
  return (compute_queue_ != VK_NULL_HANDLE) ? std::optional(compute_queue_) : std::nullopt;
}

std::optional<VkQueue> VulkanQueueSet::GetTransferQueue() const {
  return (transfer_queue_ != VK_NULL_HANDLE) ? std::optional(transfer_queue_) : std::nullopt;
}

}  // namespace liger::rhi