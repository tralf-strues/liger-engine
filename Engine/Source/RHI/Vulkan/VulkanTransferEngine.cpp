/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanTransferEngine.cpp
 * @date 2024-05-14
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

#include "VulkanTransferEngine.hpp"

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanUtils.hpp"

namespace liger::rhi {

VulkanTransferEngine::VulkanTransferEngine(VulkanDevice& device) : device_(device) {}

VulkanTransferEngine::~VulkanTransferEngine() {
  staging_capacity_ = 0U;

  if (staging_buffers_[cur_idx_] != VK_NULL_HANDLE) {
    vmaUnmapMemory(device_.GetAllocator(), allocations_[cur_idx_]);
    cur_data_size_   = 0U;
    cur_mapped_data_ = nullptr;
  }

  for (uint32_t i = 0U; i < 2U; ++i) {
    if (command_pools_[i] != VK_NULL_HANDLE) {
      vkDestroyCommandPool(device_.GetVulkanDevice(), command_pools_[i], nullptr);
      command_pools_[i] = VK_NULL_HANDLE;
    }

    if (staging_buffers_[i] != VK_NULL_HANDLE) {
      vmaDestroyBuffer(device_.GetAllocator(), staging_buffers_[i], allocations_[i]);
      staging_buffers_[i] = VK_NULL_HANDLE;
      allocations_[i] = VK_NULL_HANDLE;
    }
  }
}

void VulkanTransferEngine::Init(VkQueue queue, uint32_t queue_family, uint64_t staging_capacity) {
  staging_capacity_ = staging_capacity;

  for (uint32_t i = 0U; i < 2U; ++i) {
    const VkCommandPoolCreateInfo pool_info {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = 0,
      .queueFamilyIndex = queue_family
    };

    VULKAN_CALL(vkCreateCommandPool(device_.GetVulkanDevice(), &pool_info, nullptr, &command_pools_[i]));
    device_.SetDebugName(command_pools_[i], "VulkanTransferEngine::command_pools_[{0}]", i);

    const VkBufferCreateInfo buffer_info{
      .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .size                  = staging_capacity_,
      .usage                 = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
      .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices   = nullptr
    };

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VULKAN_CALL(vmaCreateBuffer(device_.GetAllocator(), &buffer_info, &alloc_info, &staging_buffers_[i], &allocations_[i], nullptr));
    device_.SetDebugName(staging_buffers_[i], "VulkanTransferEngine::staging_buffers_[{0}]", i);
  }

  Flip();
}

void VulkanTransferEngine::Request(IDevice::DedicatedTransferRequest&& transfer) {
  for (auto it = transfer.buffer_transfers.begin(); it != transfer.buffer_transfers.end(); ++it) {
    const auto& buffer_transfer = *it;

    if (buffer_transfer.size > staging_capacity_) {
      LIGER_LOG_ERROR(kLogChannelRHI,
                      "Requested buffer transfer of size {0} bytes is too large. Current staging capacity is {1} bytes",
                      buffer_transfer.size, staging_capacity_);
      return;
    }

    auto new_data_size_ = cur_data_size_ + buffer_transfer.size;
    if (new_data_size_ > staging_capacity_) {
      continue;
    }

    VkBuffer dst_buffer = static_cast<VulkanBuffer*>(buffer_transfer.buffer)->GetVulkanBuffer();

    const VkBufferCopy2 copy_region {
      .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
      .pNext     = nullptr,
      .srcOffset = cur_data_size_,
      .dstOffset = 0U,
      .size      = buffer_transfer.size,
    };

    const VkCopyBufferInfo2 copy_info {
      .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
      .pNext       = nullptr,
      .srcBuffer   = staging_buffers_[cur_idx_],
      .dstBuffer   = dst_buffer,
      .regionCount = 1U,
      .pRegions    = &copy_region
    };

    std::memcpy(reinterpret_cast<uint8_t*>(cur_mapped_data_) + cur_data_size_, buffer_transfer.data.get(), buffer_transfer.size);
    cur_data_size_ = new_data_size_;

    vkCmdCopyBuffer2(cur_cmds_, &copy_info);

    transfer.buffer_transfers.erase(it);
  }

  if (transfer.buffer_transfers.empty()) {
    cur_callbacks_.emplace_back(std::move(transfer.callback));
  } else {
    pending_.emplace_back(std::move(transfer));
  }
}

void VulkanTransferEngine::SubmitAndWait() {
  // No transfers scheduled
  if (cur_data_size_ == 0U) {
    return;
  }

  vmaUnmapMemory(device_.GetAllocator(), allocations_[cur_idx_]);

  const VkCommandBufferSubmitInfo cmds_submit_info {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .pNext         = nullptr,
    .commandBuffer = cur_cmds_,
    .deviceMask    = 0
  };

  const VkSubmitInfo2 submit_info {
    .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .pNext                    = nullptr,
    .flags                    = 0,
    .waitSemaphoreInfoCount   = 0U,
    .pWaitSemaphoreInfos      = nullptr,
    .commandBufferInfoCount   = 1U,
    .pCommandBufferInfos      = &cmds_submit_info,
    .signalSemaphoreInfoCount = 0U,
    .pSignalSemaphoreInfos    = nullptr
  };

  VULKAN_CALL(vkQueueSubmit2(queue_, 1U, &submit_info, VK_NULL_HANDLE));

  Flip();  // NOTE (tralf-strues): do at least some CPU work before blocking

  vkQueueWaitIdle(queue_);

  for (auto& callback : cur_callbacks_) {
    callback();
  }

  cur_callbacks_.clear();

  ReschedulePending();
}

void VulkanTransferEngine::Flip() {
  cur_idx_ = (cur_idx_ + 1U) % 2U;
  VULKAN_CALL(vkResetCommandPool(device_.GetVulkanDevice(), command_pools_[cur_idx_], 0));

  const VkCommandBufferAllocateInfo cmds_allocate_info {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext              = nullptr,
    .commandPool        = command_pools_[cur_idx_],
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };

  VULKAN_CALL(vkAllocateCommandBuffers(device_.GetVulkanDevice(), &cmds_allocate_info, &cur_cmds_));

  VULKAN_CALL(vmaMapMemory(device_.GetAllocator(), allocations_[cur_idx_], &cur_mapped_data_));
  cur_data_size_ = 0U;
}

void VulkanTransferEngine::ReschedulePending() {
  for (auto it = pending_.begin(); it != pending_.end(); ++it) {
    Request(std::move(*it));
    pending_.erase(it);
  }
}

}  // namespace liger::rhi