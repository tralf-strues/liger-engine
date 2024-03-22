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

#include <liger/rhi/vulkan/vulkan_command_pool.hpp>
#include <liger/rhi/vulkan/vulkan_device.hpp>
#include <liger/rhi/vulkan/vulkan_timeline_semaphore.hpp>

#include <vector>

namespace liger::rhi {

class VulkanRenderGraph : public RenderGraph {
 public:
  static constexpr uint32_t kMaxQueuesSupported = 3;

  ~VulkanRenderGraph() override = default;

  void ReimportTexture(ResourceVersion version, TextureResource new_texture) override;

  void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) override;

  void Execute(VkSemaphore wait, uint64_t wait_value, VkSemaphore signal, uint64_t signal_value);

  void DumpGraphviz(std::string_view filename) override;

 private:
  struct VulkanNode {
    VkRenderingInfoKHR* rendering_info   = nullptr;
    uint32_t            queue_idx        = 0;
    DependencyLevel     dependency_level = 0;

    uint32_t in_image_barrier_begin_idx  = 0;
    uint32_t in_image_barrier_count      = 0;
    uint32_t out_image_barrier_begin_idx = 0;
    uint32_t out_image_barrier_count     = 0;

    uint32_t in_buffer_barrier_begin_idx  = 0;
    uint32_t in_buffer_barrier_count      = 0;
    uint32_t out_buffer_barrier_begin_idx = 0;
    uint32_t out_buffer_barrier_count     = 0;
  };

  struct Submit {
    struct SemaphoreInfo {
      uint64_t              base_value = 0;
      VkPipelineStageFlags2 stages     = VK_PIPELINE_STAGE_2_NONE;
    };

    uint32_t                                       dependency_level;
    std::array<SemaphoreInfo, kMaxQueuesSupported> wait_per_queue;
    SemaphoreInfo                                  signal;
  };

  void Compile(IDevice& device) override;

  bool UpdateDependentResourceValues();
  void RecreateTransientResources();
  void SetupAttachments();
  void CalculateRenderPassCount(size_t& render_pass_count, size_t& total_attachment_count) const;

  void ScheduleToQueues();
  void SetupBarriers();
  void LinkBarriersToResources();
  void CreateSemaphores();

  VulkanNode& GetVulkanNode(const Node& node);
  VulkanNode& GetVulkanNode(NodeHandle node_handle);
  NodeHandle GetNodeHandle(const VulkanNode& vulkan_node);

  uint64_t GetSemaphoreValue(uint32_t queue_idx, uint64_t base_value) const;
  static VkPipelineStageFlags2 GetVulkanPipelineSrcStage(Node::Type node_type, DeviceResourceState resource_state);
  static VkPipelineStageFlags2 GetVulkanPipelineDstStage(Node::Type node_type, DeviceResourceState resource_state);

  VulkanDevice* device_{nullptr};
  bool          dirty_{false};
  bool          first_frame_{true};

  std::vector<VulkanNode> vulkan_nodes_;

  std::vector<std::unique_ptr<ITexture>> transient_textures_;
  std::vector<std::unique_ptr<IBuffer>>  transient_buffers_;

  std::vector<VkRenderingInfo>           vk_rendering_infos_;
  std::vector<VkRenderingAttachmentInfo> vk_attachments_;

  uint32_t                                 queue_count_{1};
  std::array<VkQueue, kMaxQueuesSupported> vk_queues_;

  VulkanCommandPool command_pool_;

  std::array<std::vector<VulkanNode*>, kMaxQueuesSupported> nodes_per_queue_;
  std::array<std::vector<Submit>, kMaxQueuesSupported>      submits_per_queue_;
  std::array<VulkanTimelineSemaphore, kMaxQueuesSupported>  semaphores_per_queue_;

  std::vector<VkImageMemoryBarrier2>  vk_image_barriers_;
  std::vector<ResourceId>             image_barrier_resources_;
  std::vector<VkBufferMemoryBarrier2> vk_buffer_barriers_;
};

}  // namespace liger::rhi