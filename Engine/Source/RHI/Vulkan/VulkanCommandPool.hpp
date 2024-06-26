/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanCommandPool.hpp
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

#include "VulkanCommandBuffer.hpp"
#include "VulkanQueueSet.hpp"

namespace liger::rhi {

class VulkanDevice;

class VulkanCommandPool {
 public:
  VulkanCommandPool() = default;
  ~VulkanCommandPool();

  void Init(VulkanDevice& device, uint32_t frames_in_flight, VkDescriptorSet ds, const VulkanQueueSet& queue_set,
            bool use_debug_labels);
  void Destroy();

  VulkanCommandBuffer AllocateCommandBuffer(uint32_t frame_idx, uint32_t queue_idx);

  void Reset(uint32_t frame_idx);

 private:
  struct CommandBufferList {
    size_t                           cur_idx{0U};
    std::vector<VulkanCommandBuffer> command_buffers;
  };

  VkCommandPool& GetCommandPool(uint32_t frame_idx, uint32_t queue_idx);
  CommandBufferList& GetCommandBufferList(uint32_t frame_idx, uint32_t queue_idx);

  VulkanDevice*                  device_{nullptr};
  bool                           use_debug_labels_{false};
  uint32_t                       frames_in_flight_{0};
  uint32_t                       queue_count_{0};
  std::vector<VkCommandPool>     pools_;
  std::vector<CommandBufferList> command_buffers_per_pool_;
};

}  // namespace liger::rhi