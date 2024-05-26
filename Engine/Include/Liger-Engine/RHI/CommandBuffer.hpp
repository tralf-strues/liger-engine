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
#include <Liger-Engine/RHI/DeviceResourceState.hpp>
#include <Liger-Engine/RHI/Extent.hpp>
#include <Liger-Engine/RHI/Filter.hpp>

#include <fmt/format.h>
#include <glm/glm.hpp>

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

/* Indirect commands FIXME (tralf-strues): redesign in order to support APIs other than just Vulkan */
struct DrawCommand {
  uint32_t index_count;
  uint32_t instance_count;
  uint32_t first_index;
  int32_t  vertex_offset;
  uint32_t first_instance;
};

class IBuffer;
class IPipeline;
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
   * @param final_state
   * @param filter
   */
  virtual void GenerateMipLevels(ITexture* texture, DeviceResourceState final_state, Filter filter) = 0;

  /**
   * @brief Set buffer barrier to transition it from src state to dst state.
   *
   * @note Must only be used to transition buffer's state within this command buffer stream, i.e. within a single render graph node.
   *
   * @param buffer
   * @param src_state
   * @param dst_state
   */
  virtual void BufferBarrier(const IBuffer* buffer, DeviceResourceState src_state, DeviceResourceState dst_state) = 0;

  /**
   * @brief Set the push constant for the pipeline.
   *
   * @note If compute pipeline, then command capabilities must contain @ref Capability::Compute!
   * @note If graphics pipeline, then command capabilities must contain @ref Capability::Graphics!
   *
   * @param pipeline
   * @param data
   */
  virtual void SetPushConstant(const IPipeline* pipeline, std::span<const char> data) = 0;

  /**
   * @brief Bind pipeline.
   *
   * @note If compute pipeline, then command capabilities must contain @ref Capability::Compute!
   * @note If graphics pipeline, then command capabilities must contain @ref Capability::Graphics!
   *
   * @param pipeline
   */
  virtual void BindPipeline(const IPipeline* pipeline) = 0;

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

  virtual void DrawIndexedIndirect(const IBuffer* indirect_buffer, uint64_t offset, uint64_t stride, uint32_t draw_count) = 0;

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

  /**
   * @brief Open a debug label region, marking all commands in this region until closing.
   *
   * @note In order for this function to work, device must be created with at least basic validation.
   *
   * @param name
   * @param color
   */
  virtual void BeginDebugLabelRegion(std::string_view name, const glm::vec4& color) = 0;

  template <typename... FormatArgs>
  void BeginDebugLabelRegion(const glm::vec4& color, std::string_view fmt, FormatArgs&&... args) {
    BeginDebugLabelRegion(fmt::format(fmt::runtime(fmt), std::forward<FormatArgs>(args)...), color);
  }

  /**
   * @brief Close the debug label region.
   *
   * @note In order for this function to work, device must be created with at least basic validation.
   */
  virtual void EndDebugLabelRegion() = 0;
};

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::ICommandBuffer::Capability);
