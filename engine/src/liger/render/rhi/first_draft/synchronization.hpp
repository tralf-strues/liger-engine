/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file synchronization.hpp
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
#include <liger/render/rhi/graphics_pipeline.hpp>
#include <liger/render/rhi/texture.hpp>

namespace liger::rhi {

/**
 * @brief Synchronization primitive which can be used for sync between command queues and/or the CPU.
 */
class Fence final : DeviceResource {
 public:
  explicit Fence(DeviceResource::InternalHandle internal = nullptr);
  ~Fence() override = default;  
};

Fence::Fence(DeviceResource::InternalHandle internal) : DeviceResource(internal) {}

/* Buffer Barrier */
struct BufferBarrierInfo {
  const Buffer*       buffer;
  uint64_t            offset;
  uint64_t            size;

  PipelineStages      src_stages;
  MemoryAccess        src_access;

  PipelineStages      dst_stages;
  MemoryAccess        dst_access;

  const CommandQueue* src_queue{nullptr};
  const CommandQueue* dst_queue{nullptr};
};

/* Texture Barrier */
struct TextureBarrierInfo {
  const Texture*      texture;
  uint32_t            first_mip;
  uint32_t            mip_count;

  uint32_t            first_layer;
  uint32_t            layer_count;

  PipelineStages      src_stages;
  MemoryAccess        src_access;

  PipelineStages      dst_stages;
  MemoryAccess        dst_access;

  TextureLayout       old_layout;
  TextureLayout       new_layout;

  const CommandQueue* src_queue{nullptr};
  const CommandQueue* dst_queue{nullptr};
};

}  // namespace liger::rhi