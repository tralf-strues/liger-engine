/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_command_buffer.cpp
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

#include <liger/rhi/vulkan/vulkan_command_buffer.hpp>

#include <liger/rhi/vulkan/vulkan_buffer.hpp>
#include <liger/rhi/vulkan/vulkan_compute_pipeline.hpp>
#include <liger/rhi/vulkan/vulkan_graphics_pipeline.hpp>
#include <liger/rhi/vulkan/vulkan_texture.hpp>

namespace liger::rhi {

VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer vk_cmds) : vk_cmds_(vk_cmds) {}

VkCommandBuffer VulkanCommandBuffer::Get() {
  return vk_cmds_;
}

void VulkanCommandBuffer::Begin() {
  const VkCommandBufferBeginInfo begin_info {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext            = nullptr,
    .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr
  };

  VULKAN_CALL(vkBeginCommandBuffer(vk_cmds_, &begin_info));
}

void VulkanCommandBuffer::End() {
  VULKAN_CALL(vkEndCommandBuffer(vk_cmds_));
}

void VulkanCommandBuffer::GenerateMipLevels(ITexture* /*texture*/, Filter /*filter*/) {
  LIGER_ASSERT(false, kLogChannelRHI, "Not implemented!");
}

void VulkanCommandBuffer::SetPushConstant(const IComputePipeline* compute_pipeline, std::span<const char> data) {
  const auto& vulkan_pipeline = static_cast<const VulkanComputePipeline&>(*compute_pipeline);

  vkCmdPushConstants(vk_cmds_, vulkan_pipeline.GetVulkanLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, data.size(),
                     data.data());
}

void VulkanCommandBuffer::SetPushConstant(const IGraphicsPipeline* graphics_pipeline, std::span<const char> data) {
  const auto& vulkan_pipeline = static_cast<const VulkanGraphicsPipeline&>(*graphics_pipeline);

  vkCmdPushConstants(vk_cmds_, vulkan_pipeline.GetVulkanLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, data.size(), data.data());
}

void VulkanCommandBuffer::BindPipeline(const IComputePipeline* compute_pipeline) {
  const auto& vulkan_pipeline = static_cast<const VulkanComputePipeline&>(*compute_pipeline);
  vkCmdBindPipeline(vk_cmds_, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan_pipeline.GetVulkanPipeline());
}

void VulkanCommandBuffer::BindPipeline(const IGraphicsPipeline* graphics_pipeline) {
  const auto& vulkan_pipeline = static_cast<const VulkanGraphicsPipeline&>(*graphics_pipeline);
  vkCmdBindPipeline(vk_cmds_, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_pipeline.GetVulkanPipeline());
}

void VulkanCommandBuffer::Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) {
  vkCmdDispatch(vk_cmds_, group_count_x, group_count_y, group_count_z);
}

void VulkanCommandBuffer::SetViewports(std::span<const Viewport> viewports) {
  vkCmdSetViewport(vk_cmds_, 0, viewports.size(), reinterpret_cast<const VkViewport*>(viewports.data()));
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