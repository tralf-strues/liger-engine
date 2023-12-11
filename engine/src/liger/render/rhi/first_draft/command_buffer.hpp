/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file command_buffer.hpp
 * @date 2023-11-05
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

#include <liger/render/rhi/buffer.hpp>
#include <liger/render/rhi/command_queue.hpp>
#include <liger/render/rhi/compute_pipeline.hpp>
#include <liger/render/rhi/graphics_pipeline.hpp>
#include <liger/render/rhi/render_pass.hpp>
#include <liger/render/rhi/sampler.hpp>
#include <liger/render/rhi/synchronization.hpp>
#include <liger/render/rhi/texture.hpp>

#include <span>
#include "liger/render/rhi/extent.hpp"

namespace liger::rhi {

/* Viewport */
struct Viewport {
  float x         {0.0f};  ///< In pixels
  float y         {0.0f};  ///< In pixels
  float width     {0.0f};  ///< In pixels
  float height    {0.0f};  ///< In pixels
  float min_depth {0.0f};  ///< Normalized
  float max_depth {1.0f};  ///< Normalized
};

/* Render Area */
struct RenderArea {
  Extent2D offset {0};  ///< In pixels
  Extent2D extent {0};  ///< In pixels
};

class CommandBuffer {
 public:
  virtual ~CommandBuffer() = default;

  virtual void Begin() = 0;
  virtual void End() = 0;

  /************************************************************************************************
   * Synchronization
   ************************************************************************************************/
  virtual void SetBarriers(std::span<const BufferBarrierInfo>  buffer_barriers,
                           std::span<const TextureBarrierInfo> texture_barriers) = 0;

  /************************************************************************************************
   * Graphics Commands
   ************************************************************************************************/
  virtual void GenerateMips(const Texture& texture,
                            Filter filter = Filter::kLinear,
                            TextureLayout final_layout = TextureLayout::kReadOnly) = 0;

  struct RenderPassBeginInfo {
    RenderPass  render_pass;
    Framebuffer framebuffer;
    RenderArea  render_area;

    /**
     * @brief Clear values indexed by attachment number.
     * @note If attachment i doesn't have load_op set to AttachmentLoadOperation::kClear,
     *       then clear value i is ignored.
     */
    std::span<ClearValue> clear_values;
  };

  virtual void BeginRenderPass(const RenderPassBeginInfo& begin_info) = 0;
  virtual void EndRenderPass() = 0;

  virtual void SetPushConstant(const GraphicsPipeline& pipeline, std::span<const void> data) = 0;

  virtual void BindGraphicsPipeline(const GraphicsPipeline& pipeline) = 0;

  virtual void SetViewports(std::span<const Viewport> viewports) = 0;

  virtual void BindVertexBuffers(uint32_t first_binding, std::span<const Buffer> vertex_buffers) = 0;

  void BindVertexBuffer(uint32_t binding, const Buffer& vertex_buffer) {
    BindVertexBuffers(binding, std::span<const Buffer>(&vertex_buffer, 1));
  }

  virtual void BindIndexBuffer(const Buffer& index_buffer) = 0;

  virtual void Draw(uint32_t vertices_count,
                    uint32_t first_vertex = 0,
                    uint32_t instances_count = 1,
                    uint32_t first_instance = 0) = 0;

  virtual void DrawIndexed(uint32_t indices_count,
                           uint32_t first_index     = 0,
                           uint32_t vertex_offset   = 0,
                           uint32_t instances_count = 1,
                           uint32_t first_instance  = 0) = 0;

  /************************************************************************************************
   * Compute Commands
   ************************************************************************************************/
  virtual void BindComputePipeline(const ComputePipeline& pipeline) = 0;

  virtual void Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) = 0;

  /************************************************************************************************
   * Transfer Commands
   ************************************************************************************************/
  virtual void CopyBuffer(const Buffer& src_buffer, Buffer& dst_buffer, uint64_t size, uint64_t src_offset = 0,
                          uint64_t dst_offset = 0) = 0;

  /**
   * @brief Copy data from the buffer to the texture.
   * 
   * @param buffer 
   * @param texture 
   * @param layout 
   * @param extent 
   * @param mip_level
   * 
   * @warning Texture must be in either @ref{TextureLayout::kTransferDst} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyBufferToTexture(const Buffer& buffer, Texture& texture, TextureLayout layout, Extent3D extent,
                                   uint32_t mip_level = 0) = 0;

  /**
   * @brief Copy data from the texture to the buffer.
   *
   * @param texture
   * @param buffer
   * @param layout
   * @param extent
   * @param mip_level
   *
   * @warning Texture must be in either @ref{TextureLayout::kTransferSrc} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyTextureToBuffer(const Texture& texture, Buffer& buffer, TextureLayout layout, Extent3D extent,
                                   uint32_t mip_level = 0) = 0;

  /**
   * @brief Copy data from the src to the dst texture.
   *
   * @param src_texture
   * @param dst_texture
   * @param extent
   * @param offset
   * @param src_mip_level
   * @param dst_mip_level
   *
   * @warning src_texture must be in either @ref{TextureLayout::kTransferSrc} or @ref{TextureLayout::kGeneral} layouts.
   * @warning dst_texture must be in either @ref{TextureLayout::kTransferDst} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyTexture(const Texture& src_texture, Texture& dst_texture, TextureLayout src_layout,
                           TextureLayout dst_layout, Extent3D extent, Extent3D offset = Extent3D{},
                           uint32_t src_mip_level = 0, uint32_t dst_mip_level = 0) = 0;
};

}  // namespace liger::rhi
