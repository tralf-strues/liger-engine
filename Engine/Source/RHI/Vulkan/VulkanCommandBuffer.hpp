/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanCommandBuffer.hpp
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

#pragma once

#include <Liger-Engine/RHI/CommandBuffer.hpp>

#include "VulkanUtils.hpp"

namespace liger::rhi {

class VulkanCommandBuffer : public ICommandBuffer {
 public:
  static constexpr uint32_t kMaxBindVertexBuffers = 8;

  explicit VulkanCommandBuffer(VkCommandBuffer vk_cmds);
  ~VulkanCommandBuffer() override = default;

  VkCommandBuffer Get();

  void Begin();
  void End();

  void GenerateMipLevels(ITexture* texture, Filter filter) override;

  void SetPushConstant(const IPipeline* pipeline, std::span<const char> data) override;

  void BindPipeline(const IPipeline* pipeline) override;

  void Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) override;

  void SetViewports(std::span<const Viewport> viewports) override;

  void BindVertexBuffers(uint32_t first_binding, std::span<const IBuffer*> vertex_buffers) override;
  void BindIndexBuffer(const IBuffer* index_buffer) override;

  void Draw(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) override;
  void DrawIndexed(uint32_t index_count, uint32_t first_index, uint32_t vertex_offset, uint32_t instance_count,
                   uint32_t first_instance) override;

  void CopyBuffer(const IBuffer* src_buffer, IBuffer* dst_buffer, uint64_t size, uint64_t src_offset,
                  uint64_t dst_offset) override;
  void CopyBufferToTexture(const IBuffer* buffer, ITexture* texture, Extent3D extent, uint32_t mip_level) override;
  void CopyTextureToBuffer(const ITexture* texture, IBuffer* buffer, Extent3D extent, uint32_t mip_level) override;
  void CopyTexture(const ITexture* src_texture, ITexture* dst_texture, Extent3D extent, uint32_t src_mip_level,
                   uint32_t dst_mip_level) override;

 private:
  VkCommandBuffer vk_cmds_{VK_NULL_HANDLE};
};

}  // namespace liger::rhi