/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_render_graph.hpp
 * @date 2024-02-16
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

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <liger/render/rhi/vulkan/vulkan_device.hpp>

#include <vector>

namespace liger::rhi {

class VulkanRenderGraph : public RenderGraph {
 public:
  static constexpr uint32_t kMaxQueuesSupported = 3;

  explicit VulkanRenderGraph();
  ~VulkanRenderGraph() override;

  void ReimportTexture(ResourceVersion version, TextureResource new_texture) override;

  void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) override;

  void Execute(IDevice& device) override;

 private:
  struct VulkanNode {
    VkRenderingInfoKHR* rendering_info{nullptr};

    uint32_t queue_idx = 0;

    size_t image_barrier_begin_idx = 0;
    size_t image_barrier_count = 0;

    size_t buffer_barrier_begin_idx = 0;
    size_t buffer_barrier_count = 0;
  };

//   struct ExecutionCommand {
//     VulkanNode* node{nullptr};

//     std::array<VkSemaphoreSubmitInfo, kMaxQueuesSupported> wait_per_queue;
//     VkSemaphoreSubmitInfo signal;
//   };

  struct Submit {
    uint32_t                                               dependency_level;
    std::array<VkSemaphoreSubmitInfo, kMaxQueuesSupported> wait_per_queue;
    VkSemaphoreSubmitInfo                                  signal;
  };

  void Compile(IDevice& device) override;

  void CreateTransientResources(IDevice& device);
  void SetupAttachments();
  void CalculateRenderPassCount(size_t& render_pass_count, size_t& total_attachment_count) const;

  void ScheduleToQueues(VulkanDevice& device);

  VulkanNode& GetVulkanNode(const Node& node);
  VulkanNode& GetVulkanNode(NodeHandle node_handle);
  NodeHandle GetNodeHandle(const VulkanNode& vulkan_node);

  std::vector<VulkanNode> vulkan_nodes_;

  std::vector<std::unique_ptr<ITexture>> transient_textures_;
  std::vector<std::unique_ptr<IBuffer>>  transient_buffers_;

  std::vector<VkRenderingInfo>           vk_rendering_infos_;
  std::vector<VkRenderingAttachmentInfo> vk_attachments_;

  std::array<VkQueue, kMaxQueuesSupported> vk_queues_;

  std::array<std::vector<VulkanNode*>, kMaxQueuesSupported> nodes_per_queue_;
  std::array<std::vector<Submit>, kMaxQueuesSupported>      submits_per_queue_;

  std::vector<VkImageMemoryBarrier2>  vk_image_barriers_;
  std::vector<VkBufferMemoryBarrier2> vk_buffer_barriers_;
};

}  // namespace liger::rhi