/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file CommandBuffer.hpp
 * @date 2024-02-03
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

#include <Liger-Engine/Core/EnumBitmask.hpp>
#include <Liger-Engine/RHI/Extent.hpp>
#include <Liger-Engine/RHI/Filter.hpp>

#include <span>

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
  Extent2D offset{0};  ///< In pixels
  Extent2D extent{0};  ///< In pixels
};

class IBuffer;
class IComputePipeline;
class IGraphicsPipeline;
class ITexture;

class ICommandBuffer {
 public:
  enum class Capability : uint8_t {
    None     = 0,
    Graphics = Bit(0),
    Compute  = Bit(1),
    Transfer = Bit(2),
  };

  virtual ~ICommandBuffer() = default;

  /**
   * @brief Generate @ref ITexture::Info::mip_levels number of mip levels.
   *
   * @note Command capabilities must contain @ref Capability::Graphics and @ref Capability::Transfer!
   *
   * @param texture
   * @param filter
   */
  virtual void GenerateMipLevels(ITexture* texture, Filter filter = Filter::Linear) = 0;

  /**
   * @brief Set the push constant for the compute pipeline.
   *
   * @note Command capabilities must contain @ref Capability::Compute!
   *
   * @param compute_pipeline
   * @param data
   */
  virtual void SetPushConstant(const IComputePipeline* compute_pipeline, std::span<const char> data) = 0;

  /**
   * @brief Set the push constant for the graphics pipeline.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param graphics_pipeline
   * @param data
   */
  virtual void SetPushConstant(const IGraphicsPipeline* graphics_pipeline, std::span<const char> data) = 0;

  /**
   * @brief Bind compute pipeline.
   *
   * @note Command capabilities must contain @ref Capability::Compute!
   *
   * @param compute_pipeline
   */
  virtual void BindPipeline(const IComputePipeline* compute_pipeline) = 0;

  /**
   * @brief Bind graphics pipeline.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param graphics_pipeline
   */
  virtual void BindPipeline(const IGraphicsPipeline* graphics_pipeline) = 0;

  /**
   * @brief Compute dispatch call.
   *
   * @note Command capabilities must contain @ref Capability::Compute!
   *
   * @param group_count_x
   * @param group_count_y
   * @param group_count_z
   */
  virtual void Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) = 0;

  /**
   * @brief Set viewports for color targets in current render pass.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param viewports
   */
  virtual void SetViewports(std::span<const Viewport> viewports) = 0;

  /**
   * @brief Bind several vertex buffers.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param first_binding
   * @param vertex_buffers
   */
  virtual void BindVertexBuffers(uint32_t first_binding, std::span<const IBuffer*> vertex_buffers) = 0;

  /**
   * @brief A convenience method for binding a single vertex buffer.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param binding
   * @param vertex_buffer
   */
  void BindVertexBuffer(uint32_t binding, const IBuffer* vertex_buffer)  {
    BindVertexBuffers(binding, std::span<const IBuffer*>(&vertex_buffer, 1));
  }

  /**
   * @brief Bind index buffer.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param index_buffer
   */
  virtual void BindIndexBuffer(const IBuffer* index_buffer) = 0;

  /**
   * @brief Draw call without an index buffer bound.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param vertices_count
   * @param first_vertex
   * @param instances_count
   * @param first_instance
   */
  virtual void Draw(uint32_t vertices_count, uint32_t first_vertex = 0, uint32_t instances_count = 1,
                    uint32_t first_instance = 0) = 0;

  /**
   * @brief Draw call with an index buffer bound.
   *
   * @note Command capabilities must contain @ref Capability::Graphics!
   *
   * @param indices_count
   * @param first_index
   * @param vertex_offset
   * @param instances_count
   * @param first_instance
   */
  virtual void DrawIndexed(uint32_t indices_count, uint32_t first_index = 0, uint32_t vertex_offset = 0,
                           uint32_t instances_count = 1, uint32_t first_instance = 0) = 0;

  /**
   * @brief Copy a region of src buffer's memory to dst buffer's memory.
   *
   * @note Command capabilities must contain @ref Capability::Transfer!
   *
   * @param src_buffer
   * @param dst_buffer
   * @param size
   * @param src_offset
   * @param dst_offset
   */
  virtual void CopyBuffer(const IBuffer* src_buffer, IBuffer* dst_buffer, uint64_t size, uint64_t src_offset = 0,
                          uint64_t dst_offset = 0) = 0;

  /**
   * @brief Copy data from the buffer to the texture.
   *
   * @note Command capabilities must contain @ref Capability::Transfer!
   *
   * @param buffer
   * @param texture
   * @param extent
   * @param mip_level
   */
  virtual void CopyBufferToTexture(const IBuffer* buffer, ITexture* texture, Extent3D extent,
                                   uint32_t mip_level = 0) = 0;

  /**
   * @brief Copy data from the texture to the buffer.
   *
   * @note Command capabilities must contain @ref Capability::Transfer!
   *
   * @param texture
   * @param buffer
   * @param extent
   * @param mip_level
   */
  virtual void CopyTextureToBuffer(const ITexture* texture, IBuffer* buffer, Extent3D extent,
                                   uint32_t mip_level = 0) = 0;

  /**
   * @brief Copy data from the src to the dst texture.
   *
   * @note Command capabilities must contain @ref Capability::Transfer!
   *
   * @param src_texture
   * @param dst_texture
   * @param extent
   * @param src_mip_level
   * @param dst_mip_level
   */
  virtual void CopyTexture(const ITexture* src_texture, ITexture* dst_texture, Extent3D extent,
                           uint32_t src_mip_level = 0, uint32_t dst_mip_level = 0) = 0;
};

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::ICommandBuffer::Capability);
