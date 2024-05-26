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

  if (staging_buffers_[cur_frame_].buffer != VK_NULL_HANDLE) {
    vmaUnmapMemory(device_.GetAllocator(), staging_buffers_[cur_frame_].allocation);
    cur_data_size_   = 0U;
    cur_mapped_data_ = nullptr;
  }

  for (uint32_t i = 0U; i < 2U; ++i) {
    if (staging_buffers_[i].buffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(device_.GetAllocator(), staging_buffers_[i].buffer, staging_buffers_[i].allocation);
      staging_buffers_[i].buffer     = VK_NULL_HANDLE;
      staging_buffers_[i].allocation = VK_NULL_HANDLE;
    }
  }
}

void VulkanTransferEngine::Init(uint64_t staging_capacity) {
  staging_capacity_ = staging_capacity;

  timeline_semaphore_.Init(device_.GetVulkanDevice(), 3U);
  command_pool_.Init(device_, device_.GetFramesInFlight(), device_.GetDescriptorManager().GetSet(),
                     device_.GetQueues(), false);

  staging_buffers_.resize(device_.GetFramesInFlight());

  for (uint32_t i = 0U; i < staging_buffers_.size(); ++i) {
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

    VULKAN_CALL(vmaCreateBuffer(device_.GetAllocator(), &buffer_info, &alloc_info, &staging_buffers_[i].buffer,
                                &staging_buffers_[i].allocation, nullptr));
    device_.SetDebugName(staging_buffers_[i].buffer, "VulkanTransferEngine::staging_buffers_[{0}]", i);
  }

  Flip(device_.CurrentFrame());
}

void VulkanTransferEngine::Request(IDevice::DedicatedTransferRequest&& transfer) {
  ProcessBufferTransfers(transfer);
  ProcessTextureTransfers(transfer);

  if (transfer.buffer_transfers.empty() && transfer.texture_transfers.empty()) {
    callbacks_.emplace_back(Callback {
      .callback        = std::move(transfer.callback),
      .semaphore_value = timeline_semaphore_.TimePoint(device_.CurrentAbsoluteFrame(), 2U)
    });
  } else {
    pending_.emplace_back(std::move(transfer));
  }
}

void VulkanTransferEngine::Submit() {
  //// No transfers scheduled
  //if (cur_data_size_ == 0U) {
  //  return;
  //}

  vmaUnmapMemory(device_.GetAllocator(), staging_buffers_[cur_frame_].allocation);

  cmds_transfer_.End();
  cmds_graphics_.End();

  /* Callbacks */
  uint64_t cur_timeline_semaphore_value = timeline_semaphore_.GetValue();
  for (auto it = callbacks_.begin(); it != callbacks_.end();) {
    if (it->semaphore_value < cur_timeline_semaphore_value) {
      it->callback();
      it = callbacks_.erase(it);
    } else {
      ++it;
    }
  }

  /* Submit transfer work */
  const VkCommandBufferSubmitInfo transfer_cmds_submit_info {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .pNext         = nullptr,
    .commandBuffer = cmds_transfer_.Get(),
    .deviceMask    = 0
  };

  const VkSemaphoreSubmitInfo transfer_signal_info {
    .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .pNext       = nullptr,
    .semaphore   = timeline_semaphore_.Get(),
    .value       = timeline_semaphore_.TimePoint(device_.CurrentAbsoluteFrame(), 1U),
    .stageMask   = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    .deviceIndex = 0,
  };

  const VkSubmitInfo2 transfer_submit_info {
    .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .pNext                    = nullptr,
    .flags                    = 0,
    .waitSemaphoreInfoCount   = 0U,
    .pWaitSemaphoreInfos      = nullptr,
    .commandBufferInfoCount   = 1U,
    .pCommandBufferInfos      = &transfer_cmds_submit_info,
    .signalSemaphoreInfoCount = 1U,
    .pSignalSemaphoreInfos    = &transfer_signal_info
  };

  VULKAN_CALL(vkQueueSubmit2(*device_.GetQueues().GetTransferQueue(), 1U, &transfer_submit_info, VK_NULL_HANDLE));

  /* Submit graphics work */
  const VkCommandBufferSubmitInfo graphics_cmds_submit_info {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .pNext         = nullptr,
    .commandBuffer = cmds_graphics_.Get(),
    .deviceMask    = 0
  };

  const VkSemaphoreSubmitInfo graphics_wait_info {
    .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .pNext       = nullptr,
    .semaphore   = timeline_semaphore_.Get(),
    .value       = timeline_semaphore_.TimePoint(device_.CurrentAbsoluteFrame(), 1U),
    .stageMask   = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
    .deviceIndex = 0,
  };

  const VkSemaphoreSubmitInfo graphics_signal_info {
    .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .pNext       = nullptr,
    .semaphore   = timeline_semaphore_.Get(),
    .value       = timeline_semaphore_.TimePoint(device_.CurrentAbsoluteFrame(), 2U),
    .stageMask   = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    .deviceIndex = 0,
  };

  const VkSubmitInfo2 graphics_submit_info {
    .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .pNext                    = nullptr,
    .flags                    = 0,
    .waitSemaphoreInfoCount   = 1U,
    .pWaitSemaphoreInfos      = &graphics_wait_info,
    .commandBufferInfoCount   = 1U,
    .pCommandBufferInfos      = &graphics_cmds_submit_info,
    .signalSemaphoreInfoCount = 1U,
    .pSignalSemaphoreInfos    = &graphics_signal_info
  };

  VULKAN_CALL(vkQueueSubmit2(device_.GetQueues().GetMainQueue(), 1U, &graphics_submit_info, VK_NULL_HANDLE));

  Flip(device_.NextFrame());

  ReschedulePending();
}

void VulkanTransferEngine::Flip(uint32_t next_frame) {
  const uint32_t frames_in_flight = device_.GetFramesInFlight();

  if (device_.CurrentAbsoluteFrame() >= frames_in_flight - 1U) {
    const VkSemaphore wait_semaphore = timeline_semaphore_.Get();
    const uint64_t    wait_value     = timeline_semaphore_.TimePoint(device_.CurrentAbsoluteFrame() + 1U - frames_in_flight, 2U);

    const VkSemaphoreWaitInfo wait_info {
      .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
      .pNext          = nullptr,
      .flags          = 0,
      .semaphoreCount = 1U,
      .pSemaphores    = &wait_semaphore,
      .pValues        = &wait_value
    };

    vkWaitSemaphores(device_.GetVulkanDevice(), &wait_info, UINT64_MAX);
  }

  cur_frame_ = next_frame;

  command_pool_.Reset(cur_frame_);
  cmds_transfer_ = command_pool_.AllocateCommandBuffer(cur_frame_, 2U);
  cmds_graphics_ = command_pool_.AllocateCommandBuffer(cur_frame_, 0U);

  cmds_transfer_.Begin();
  cmds_graphics_.Begin();

  VULKAN_CALL(vmaMapMemory(device_.GetAllocator(), staging_buffers_[cur_frame_].allocation, &cur_mapped_data_));
  cur_data_size_ = 0U;
}

void VulkanTransferEngine::ReschedulePending() {
  if (pending_.empty()) {
    return;
  }

  size_t max_iterations = pending_.size();
  size_t cur_iteration  = 0U;
  for (auto it = pending_.begin(); it != pending_.end() && cur_iteration < max_iterations; ++cur_iteration) {
    Request(std::move(*it));
    it = pending_.erase(it);

    if (pending_.size() <= 1U) {
      break;
    }
  }
}

void VulkanTransferEngine::ProcessBufferTransfers(IDevice::DedicatedTransferRequest& transfer) {
  for (auto it = transfer.buffer_transfers.begin(); it != transfer.buffer_transfers.end();) {
    const auto& buffer_transfer = *it;

    if (buffer_transfer.size > staging_capacity_) {
      LIGER_LOG_ERROR(kLogChannelRHI,
                      "Requested buffer transfer of size {0} bytes is too large. Current staging capacity is {1} bytes",
                      buffer_transfer.size, staging_capacity_);
      return;
    }

    auto new_data_size_ = cur_data_size_ + buffer_transfer.size;
    if (new_data_size_ > staging_capacity_) {
      ++it;
      continue;
    }

    VkBuffer dst_buffer = static_cast<VulkanBuffer*>(buffer_transfer.buffer)->GetVulkanBuffer();

    const VkBufferCopy2 copy_region {
      .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
      .pNext     = nullptr,
      .srcOffset = cur_data_size_,
      .dstOffset = 0U,
      .size      = buffer_transfer.size
    };

    const VkCopyBufferInfo2 copy_info {
      .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
      .pNext       = nullptr,
      .srcBuffer   = staging_buffers_[cur_frame_].buffer,
      .dstBuffer   = dst_buffer,
      .regionCount = 1U,
      .pRegions    = &copy_region
    };

    std::memcpy(reinterpret_cast<uint8_t*>(cur_mapped_data_) + cur_data_size_, buffer_transfer.data.get(),
                buffer_transfer.size);
    cur_data_size_ = new_data_size_;

    vkCmdCopyBuffer2(cmds_transfer_.Get(), &copy_info);

    it = transfer.buffer_transfers.erase(it);
  }
}

void VulkanTransferEngine::ProcessTextureTransfers(IDevice::DedicatedTransferRequest& transfer) {
  for (auto it = transfer.texture_transfers.begin(); it != transfer.texture_transfers.end();) {
    const auto& texture_transfer = *it;

    if (texture_transfer.size > staging_capacity_) {
      LIGER_LOG_ERROR(kLogChannelRHI,
                      "Requested texture transfer of size {} bytes is too large. Current staging capacity is {} bytes",
                      texture_transfer.size, staging_capacity_);
      return;
    }

    const uint64_t alignment = 4U;
    uint64_t offset = uint64_t((cur_data_size_ + (alignment - 1)) / alignment) * alignment;

    auto new_data_size_ = offset + texture_transfer.size;
    if (new_data_size_ > staging_capacity_) {
      ++it;
      continue;
    }

    VkImage dst_image = static_cast<VulkanTexture*>(texture_transfer.texture)->GetVulkanImage();

    /* Transfer image layout to TransferDst */
    VkImageMemoryBarrier2 image_barrier {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .pNext               = nullptr,
      .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
      .srcAccessMask       = VK_ACCESS_2_NONE,
      .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image               = dst_image,

      .subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0U,
        .levelCount     = 1U,
        .baseArrayLayer = 0U,
        .layerCount     = 1U
      }
    };

    const VkDependencyInfo dependency_info {
      .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .pNext                    = nullptr,
      .dependencyFlags          = 0,
      .memoryBarrierCount       = 0U,
      .pMemoryBarriers          = nullptr,
      .bufferMemoryBarrierCount = 0U,
      .pBufferMemoryBarriers    = nullptr,
      .imageMemoryBarrierCount  = 1U,
      .pImageMemoryBarriers     = &image_barrier
    };

    vkCmdPipelineBarrier2(cmds_transfer_.Get(), &dependency_info);

    /* Copy */
    const VkBufferImageCopy2 copy_region {
      .sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
      .pNext             = nullptr,
      .bufferOffset      = offset,
      .bufferRowLength   = 0U,
      .bufferImageHeight = 0U,

      .imageSubresource  = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0U,
        .baseArrayLayer = 0U,
        .layerCount     = 1U,
      },

      .imageOffset       = {.x = 0U, .y = 0U, .z = 0U},
      .imageExtent       = GetVulkanExtent3D(texture_transfer.texture->GetInfo().extent),
    };

    const VkCopyBufferToImageInfo2 copy_info {
      .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
      .pNext          = nullptr,
      .srcBuffer      = staging_buffers_[cur_frame_].buffer,
      .dstImage       = dst_image,
      .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .regionCount    = 1U,
      .pRegions       = &copy_region,
    };

    std::memcpy(reinterpret_cast<uint8_t*>(cur_mapped_data_) + offset, texture_transfer.data.get(),
                texture_transfer.size);
    cur_data_size_ = new_data_size_;

    vkCmdCopyBufferToImage2(cmds_transfer_.Get(), &copy_info);

    /* Transfer image layout to final usage */
    if (texture_transfer.gen_mips) {
      // Translate mip 0 to transfer src
      image_barrier.srcStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
      image_barrier.srcAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      image_barrier.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
      image_barrier.dstAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;
      image_barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      image_barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      image_barrier.subresourceRange.baseMipLevel = 0U;
      image_barrier.subresourceRange.levelCount   = 1U;
      image_barrier.srcQueueFamilyIndex           = *device_.GetQueues().GetQueueFamilyIndices().transfer;
      image_barrier.dstQueueFamilyIndex           = device_.GetQueues().GetQueueFamilyIndices().main;

      vkCmdPipelineBarrier2(cmds_transfer_.Get(), &dependency_info);
      vkCmdPipelineBarrier2(cmds_graphics_.Get(), &dependency_info);

      // Translate other mips to transfer dst
      image_barrier.srcStageMask                  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
      image_barrier.srcAccessMask                 = VK_ACCESS_2_NONE;
      image_barrier.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
      image_barrier.dstAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      image_barrier.oldLayout                     = VK_IMAGE_LAYOUT_UNDEFINED;
      image_barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      image_barrier.subresourceRange.baseMipLevel = 1U;
      image_barrier.subresourceRange.levelCount   = texture_transfer.texture->GetInfo().mip_levels - 1U;
      image_barrier.srcQueueFamilyIndex           = *device_.GetQueues().GetQueueFamilyIndices().transfer;
      image_barrier.dstQueueFamilyIndex           = device_.GetQueues().GetQueueFamilyIndices().main;

      vkCmdPipelineBarrier2(cmds_transfer_.Get(), &dependency_info);
      vkCmdPipelineBarrier2(cmds_graphics_.Get(), &dependency_info);

      cmds_graphics_.GenerateMipLevels(texture_transfer.texture, texture_transfer.final_state, texture_transfer.gen_mips_filter);
    }

    //image_barrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    //image_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    //image_barrier.dstStageMask  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    //image_barrier.dstAccessMask = GetVulkanAccessFlags(texture_transfer.final_state);
    //image_barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //image_barrier.newLayout     = GetVulkanImageLayout(texture_transfer.final_state);
    //vkCmdPipelineBarrier2(cmds_graphics_.Get(), &dependency_info);

    it = transfer.texture_transfers.erase(it);
  }
}

}  // namespace liger::rhi