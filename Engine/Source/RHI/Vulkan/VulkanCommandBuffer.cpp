/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanCommandBuffer.cpp
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

#include "VulkanCommandBuffer.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanTexture.hpp"

namespace liger::rhi {

VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer vk_cmds, VkDescriptorSet ds) : vk_cmds_(vk_cmds), ds_(ds) {}

VkCommandBuffer VulkanCommandBuffer::Get() {
  return vk_cmds_;
}

void VulkanCommandBuffer::Begin() {
  ds_bound_ = false;

  const VkCommandBufferBeginInfo begin_info {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext            = nullptr,
    .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr
  };

  VULKAN_CALL(vkBeginCommandBuffer(vk_cmds_, &begin_info));
}

void VulkanCommandBuffer::End() {
  ds_bound_ = false;

  VULKAN_CALL(vkEndCommandBuffer(vk_cmds_));
}

void VulkanCommandBuffer::GenerateMipLevels(ITexture* /*texture*/, Filter /*filter*/) {
  LIGER_ASSERT(false, kLogChannelRHI, "Not implemented!");
}

void VulkanCommandBuffer::BufferBarrier(const IBuffer* buffer, DeviceResourceState src_state, DeviceResourceState dst_state) {
  const auto& vulkan_buffer = static_cast<const VulkanBuffer&>(*buffer);

  const VkBufferMemoryBarrier2 barrier {
    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
    .pNext = nullptr,
    .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
    .srcAccessMask = GetVulkanAccessFlags(src_state),
    .dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    .dstAccessMask = GetVulkanAccessFlags(dst_state),
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = vulkan_buffer.GetVulkanBuffer(),
    .offset = 0U,
    .size = VK_WHOLE_SIZE,
  };

  const VkDependencyInfo dependency_info {
    .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .pNext                    = nullptr,
    .dependencyFlags          = 0,
    .memoryBarrierCount       = 0,
    .pMemoryBarriers          = nullptr,
    .bufferMemoryBarrierCount = 1U,
    .pBufferMemoryBarriers    = &barrier,
    .imageMemoryBarrierCount  = 0,
    .pImageMemoryBarriers     = nullptr
  };

  vkCmdPipelineBarrier2(vk_cmds_, &dependency_info);
}

void VulkanCommandBuffer::SetPushConstant(const IPipeline* pipeline, std::span<const char> data) {
  const auto& vulkan_pipeline = static_cast<const VulkanPipeline&>(*pipeline);
  const auto  bind_point      = vulkan_pipeline.GetVulkanBindPoint();
  const auto  shader_stages   = vulkan_pipeline.GetVulkanPushConstantStages();

  vkCmdPushConstants(vk_cmds_, vulkan_pipeline.GetVulkanLayout(), shader_stages, 0, data.size(), data.data());
}

void VulkanCommandBuffer::BindPipeline(const IPipeline* pipeline) {
  const auto& vulkan_pipeline = static_cast<const VulkanPipeline&>(*pipeline);
  const auto  bind_point      = vulkan_pipeline.GetVulkanBindPoint();

  vkCmdBindPipeline(vk_cmds_, bind_point, vulkan_pipeline.GetVulkanPipeline());

  if (!ds_bound_) {
    vkCmdBindDescriptorSets(vk_cmds_, bind_point, vulkan_pipeline.GetVulkanLayout(), 0U, 1U, &ds_, 0U, nullptr);
    // ds_bound_ = true; // FIXME (tralf-strues):
  }
}

void VulkanCommandBuffer::Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) {
  vkCmdDispatch(vk_cmds_, group_count_x, group_count_y, group_count_z);
}

void VulkanCommandBuffer::SetViewports(std::span<const Viewport> viewports) {
  const VkViewport vk_viewport {
    .x        = 0.0f,
    .y        =  viewports[0U].height,
    .width    =  viewports[0U].width,
    .height   = -viewports[0U].height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  vkCmdSetViewport(vk_cmds_, 0U, 1U, &vk_viewport);
}

void VulkanCommandBuffer::BindVertexBuffers(uint32_t first_binding, std::span<const IBuffer*> vertex_buffers) {
  std::array<VkBuffer, kMaxBindVertexBuffers> vk_buffers;
  std::array<VkDeviceSize, kMaxBindVertexBuffers> offsets;

  for (size_t i = 0; i < vertex_buffers.size(); ++i) {
    vk_buffers[i] = static_cast<const VulkanBuffer*>(vertex_buffers[i])->GetVulkanBuffer();
    offsets[i] = 0;
  }

  vkCmdBindVertexBuffers(vk_cmds_, first_binding, vertex_buffers.size(), vk_buffers.data(), offsets.data());
}

void VulkanCommandBuffer::BindIndexBuffer(const IBuffer* index_buffer) {
  auto vk_buffer = static_cast<const VulkanBuffer*>(index_buffer)->GetVulkanBuffer();

  vkCmdBindIndexBuffer(vk_cmds_, vk_buffer, 0, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::Draw(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count,
                               uint32_t first_instance) {
  vkCmdDraw(vk_cmds_, vertex_count, instance_count, first_vertex, first_instance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t index_count, uint32_t first_index, uint32_t vertex_offset,
                                      uint32_t instance_count, uint32_t first_instance) {
  vkCmdDrawIndexed(vk_cmds_, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void VulkanCommandBuffer::CopyBuffer(const IBuffer* src_buffer, IBuffer* dst_buffer, uint64_t size, uint64_t src_offset,
                                     uint64_t dst_offset) {
  auto vk_src = static_cast<const VulkanBuffer*>(src_buffer)->GetVulkanBuffer();
  auto vk_dst = static_cast<const VulkanBuffer*>(dst_buffer)->GetVulkanBuffer();

  const VkBufferCopy copy_info {
    .srcOffset = src_offset,
    .dstOffset = dst_offset,
    .size      = size
  };

  vkCmdCopyBuffer(vk_cmds_, vk_src, vk_dst, 1, &copy_info);
}

void VulkanCommandBuffer::CopyBufferToTexture(const IBuffer* buffer, ITexture* texture, Extent3D extent,
                                              uint32_t mip_level) {
  auto vk_buffer  = static_cast<const VulkanBuffer*>(buffer)->GetVulkanBuffer();
  auto vk_texture = static_cast<const VulkanTexture*>(texture)->GetVulkanImage();

  const VkBufferImageCopy copy_info {
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,

    .imageSubresource {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel       = mip_level,
      .baseArrayLayer = 0,
      .layerCount     = 1
    },

    .imageOffset {
      .x = 0,
      .y = 0,
      .z = 0,
    },

    .imageExtent = GetVulkanExtent3D(extent)
  };

  vkCmdCopyBufferToImage(vk_cmds_, vk_buffer, vk_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
}

void VulkanCommandBuffer::CopyTextureToBuffer(const ITexture* texture, IBuffer* buffer, Extent3D extent,
                                              uint32_t mip_level) {
  auto vk_buffer  = static_cast<const VulkanBuffer*>(buffer)->GetVulkanBuffer();
  auto vk_texture = static_cast<const VulkanTexture*>(texture)->GetVulkanImage();

  const VkBufferImageCopy copy_info {
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,

    .imageSubresource {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel       = mip_level,
      .baseArrayLayer = 0,
      .layerCount     = 1
    },

    .imageOffset {
      .x = 0,
      .y = 0,
      .z = 0,
    },

    .imageExtent = GetVulkanExtent3D(extent)
  };

  vkCmdCopyImageToBuffer(vk_cmds_, vk_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk_buffer, 1, &copy_info);
}

void VulkanCommandBuffer::CopyTexture(const ITexture* src_texture, ITexture* dst_texture, Extent3D extent,
                                      uint32_t src_mip_level, uint32_t dst_mip_level) {
  auto vk_src = static_cast<const VulkanTexture*>(src_texture)->GetVulkanImage();
  auto vk_dst = static_cast<const VulkanTexture*>(dst_texture)->GetVulkanImage();

  const VkImageCopy copy_info {
    .srcSubresource {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel       = src_mip_level,
      .baseArrayLayer = 0,
      .layerCount     = 1
    },

    .srcOffset {
      .x = 0,
      .y = 0,
      .z = 0,
    },

    .dstSubresource {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel       = dst_mip_level,
      .baseArrayLayer = 0,
      .layerCount     = 1
    },

    .dstOffset {
      .x = 0,
      .y = 0,
      .z = 0,
    },

    .extent = GetVulkanExtent3D(extent),
  };

  vkCmdCopyImage(vk_cmds_, vk_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1, &copy_info);
}

}  // namespace liger::rhi