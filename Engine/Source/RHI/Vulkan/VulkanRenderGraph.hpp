/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanRenderGraph.hpp
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

#include "VulkanCommandPool.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTimelineSemaphore.hpp"

#include <vector>

// XLib has macro None...
#undef None

namespace liger::rhi {

class VulkanRenderGraph : public RenderGraph {
 public:
  static constexpr uint32_t kMaxQueuesSupported = 3;

  ~VulkanRenderGraph() override = default;

  void ReimportTexture(ResourceVersion version, TextureResource new_texture) override;
  void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) override;
  void UpdateTransientTextureSamples(ResourceVersion version, uint8_t new_sample_count) override;
  void UpdateTransientBufferSize(ResourceVersion version, uint64_t new_size) override;

  void Execute(Context& context, VkSemaphore wait, uint64_t wait_value, VkSemaphore signal, uint64_t signal_value);

  void DumpGraphviz(std::string_view filename, bool detailed) override;

  static glm::vec4 GetDebugLabelColor(JobType node_type);

 private:
  struct VulkanNode {
    std::string name;

    VkRenderingInfoKHR* rendering_info         = nullptr;
    uint8_t             samples                = 1U;
    uint32_t            queue_idx              = 0;
    DependencyLevel     dependency_level       = 0;

    uint32_t in_image_barrier_begin_idx        = 0;
    uint32_t in_image_barrier_count            = 0;
    uint32_t out_image_barrier_begin_idx       = 0;
    uint32_t out_image_barrier_count           = 0;

    uint32_t in_buffer_barrier_begin_idx       = 0;
    uint32_t in_buffer_barrier_count           = 0;
    uint32_t out_buffer_barrier_begin_idx      = 0;
    uint32_t out_buffer_barrier_count          = 0;

    uint32_t in_buffer_pack_barrier_begin_idx  = 0;
    uint32_t in_buffer_pack_barrier_count      = 0;
    uint32_t out_buffer_pack_barrier_begin_idx = 0;
    uint32_t out_buffer_pack_barrier_count     = 0;    
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

  void SetBufferPackBarriers(VkCommandBuffer vk_cmds, VulkanNode& vulkan_node) const;

  VulkanDevice* device_{nullptr};
  bool          dirty_{false};
  bool          force_recreate_resources_{false};
  bool          first_frame_{true};

  std::vector<VulkanNode> vulkan_nodes_;

  std::unordered_map<ResourceId, std::unique_ptr<ITexture>> transient_textures_;
  std::unordered_map<ResourceId, std::unique_ptr<IBuffer>>  transient_buffers_;

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
  std::vector<ResourceId>             buffer_barrier_resources_;
  std::vector<VkBufferMemoryBarrier2> vk_buffer_pack_barriers_;
  std::vector<ResourceId>             buffer_pack_barrier_resources_;
};

}  // namespace liger::rhi