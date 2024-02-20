/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_render_graph.cpp
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

#include <liger/render/rhi/vulkan/vulkan_render_graph.hpp>

#include <liger/render/rhi/vulkan/vulkan_texture.hpp>

namespace liger::rhi {

void VulkanRenderGraph::ReimportTexture(ResourceVersion version, TextureResource new_texture) {}

void VulkanRenderGraph::ReimportBuffer(ResourceVersion version, BufferResource new_buffer) {}

void VulkanRenderGraph::Execute(IDevice& device) {
  
}

void VulkanRenderGraph::Compile(IDevice& device) {
  auto& vulkan_device = static_cast<VulkanDevice&>(device);

  vulkan_nodes_.resize(dag_.Size());

  CreateTransientResources(device);
  SetupAttachments();
  ScheduleToQueues(vulkan_device);
}

void VulkanRenderGraph::CreateTransientResources(IDevice& device) {
  for (const auto& [version, info] : transient_texture_infos_) {
    auto& texture = transient_textures_.emplace_back(device.CreateTexture(info));
    resource_version_registry_.UpdateResource(version, TextureResource{texture.get(), kTextureDefaultViewIdx});
  }

  for (const auto& [version, info] : transient_buffer_infos_) {
    auto& buffer = transient_buffers_.emplace_back(device.CreateBuffer(info));
    resource_version_registry_.UpdateResource(version, buffer.get());
  }
}

void VulkanRenderGraph::SetupAttachments() {
  size_t render_pass_count = 0;
  size_t total_attachment_count = 0;
  CalculateRenderPassCount(render_pass_count, total_attachment_count);

  vk_rendering_infos_.reserve(render_pass_count);
  vk_attachments_.reserve(total_attachment_count);

  for (const auto& node : dag_) {
    if (node.type != RenderGraph::Node::Type::kRenderPass) {
      continue;
    }

    Extent2D render_area;

    /* First get all color attachments */
    VkRenderingAttachmentInfo* first_color_attachment = nullptr;
    uint32_t color_attachment_count = 0;

    for (const auto& write : node.write) {
      auto resource = resource_version_registry_.TryGetResource<TextureResource>(write.version);
      if (!resource) {
        continue;
      }

      auto* texture = static_cast<VulkanTexture*>(resource->texture);

      const auto extent = texture->GetInfo().extent;
      render_area.x = extent.x;
      render_area.y = extent.y;

      if (write.state == DeviceResourceState::kColorTarget) {
        auto& attachment = vk_attachments_.emplace_back(VkRenderingAttachmentInfo {
          .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext              = nullptr,
          .imageView          = texture->GetVulkanView(resource->view),
          .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .resolveMode        = VK_RESOLVE_MODE_NONE,
          .resolveImageView   = VK_NULL_HANDLE,
          .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,  // FIXME (tralf-strues): Make customizable
          .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
          .clearValue = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}}}  // FIXME (tralf-strues): Setup once loadOp
                                                                          // is made customizable
        });

        if (first_color_attachment == nullptr) { first_color_attachment = &attachment; }

        ++color_attachment_count;
      }
    }

    /* Get depth stencil attachment */
    VkRenderingAttachmentInfo* depth_stencil_attachment = nullptr;

    for (const auto& write : node.write) {
      auto resource = resource_version_registry_.TryGetResource<TextureResource>(write.version);
      if (!resource) {
        continue;
      }

      auto* texture = static_cast<VulkanTexture*>(resource->texture);

      if (write.state == DeviceResourceState::kDepthStencilTarget) {
        auto& attachment = vk_attachments_.emplace_back(VkRenderingAttachmentInfo {
          .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext              = nullptr,
          .imageView          = texture->GetVulkanView(resource->view),
          .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .resolveMode        = VK_RESOLVE_MODE_NONE,
          .resolveImageView   = VK_NULL_HANDLE,
          .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,  // FIXME (tralf-strues): Make customizable
          .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
          .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}}   // FIXME (tralf-strues): Setup once loadOp
                                                                          // is made customizable
        });

        if (depth_stencil_attachment == nullptr) {
          depth_stencil_attachment = &attachment;
        } else {
          LIGER_LOG_ERROR(kLogChannelRHI, "There cannot be two depth stencil attachments!");
          break;
        }
      }
    }

    /* Add rendering info */
    GetVulkanNode(node).rendering_info = &vk_rendering_infos_.emplace_back(VkRenderingInfo {
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext                = nullptr,
      .flags                = 0,
      .renderArea           = VkRect2D{.offset = {0, 0}, .extent = {.width = render_area.x, .height = render_area.y}},
      .layerCount           = 1,
      .viewMask             = 0,
      .colorAttachmentCount = color_attachment_count,
      .pColorAttachments    = first_color_attachment,
      .pDepthAttachment     = depth_stencil_attachment,
      .pStencilAttachment   = depth_stencil_attachment
    });
  }
}

void VulkanRenderGraph::CalculateRenderPassCount(size_t& render_pass_count, size_t& total_attachment_count) const {
  for (const auto& node : dag_) {
    if (node.type != RenderGraph::Node::Type::kRenderPass) {
      continue;
    }

    ++render_pass_count;

    for (const auto& write : node.write) {
      if (write.state == DeviceResourceState::kColorTarget || write.state == DeviceResourceState::kDepthStencilTarget) {
        ++total_attachment_count;
      }
    }
  }
}

void VulkanRenderGraph::ScheduleToQueues(VulkanDevice& device) {
  uint32_t queue_count = 0;
  uint32_t main_queue_idx = queue_count++;
  uint32_t compute_queue_idx = (device.GetQueues().GetComputeQueue()) ? (queue_count++) : main_queue_idx;
  uint32_t transfer_queue_idx = (device.GetQueues().GetComputeQueue() ) ? (queue_count++) : main_queue_idx;

  /* Set vk_queues_ */
  vk_queues_[main_queue_idx] = device.GetQueues().GetMainQueue();

  if (auto vk_queue = device.GetQueues().GetComputeQueue(); vk_queue) {
    vk_queues_[compute_queue_idx] = *vk_queue;
  }

  if (auto vk_queue = device.GetQueues().GetTransferQueue(); vk_queue) {
    vk_queues_[transfer_queue_idx] = *vk_queue;
  }

  /* Set each Vulkan nodes' queue index */
  for (const auto& node : dag_) {
    auto& vulkan_node = GetVulkanNode(node);
    vulkan_node.queue_idx = main_queue_idx;

    if (node.type == RenderGraph::Node::Type::kCompute && node.async) {
      vulkan_node.queue_idx = compute_queue_idx;
    }

    if (node.type == RenderGraph::Node::Type::kTransfer && node.async) {
      vulkan_node.queue_idx = transfer_queue_idx;
    }
  }

  /* Build reverse graph */
  DAG<char> reverse_dag;
  for (const auto& from : dag_) {
    const auto from_handle = dag_.GetNodeHandle(from);

    for (auto to : dag_.GetAdjacencyList(from_handle)) {
      reverse_dag.AddEdge(to, from_handle);
    }
  }

  /* Order nodes monotonically through dependency levels, needed later to construct SSIS. */
  using SyncNodeIndex = uint32_t;
  std::vector<NodeHandle> handle_from_sync_index(dag_.Size());

  // FIXME (tralf-strues): currently the time complexity is theoretically atrocious O(n^3),
  // yet there aren't that many nodes practically and we're only doing this once...
  for (int32_t dependency_level = max_dependency_level_; dependency_level >= 0; --dependency_level) {
    for (const auto& node : dag_) {
      if (GetDependencyLevel(dag_.GetNodeHandle(node)) != dependency_level) {
        continue;
      }

      auto& vulkan_node = GetVulkanNode(node);
      nodes_per_queue_[vulkan_node.queue_idx].push_back(&vulkan_node);

      // Find any nodes on other queues which depend on this one.
      for (uint32_t queue_idx = 0; queue_idx < queue_count; ++queue_idx) {
        if (vulkan_node.queue_idx == queue_idx) {
          continue;
        }

        for (auto other_node : nodes_per_queue_[queue_idx]) {
          if (dag_.EdgeExists(GetNodeHandle(vulkan_node), GetNodeHandle(*other_node)) &&
              submits_per_queue_[other_node->queue_idx].back().dependency_level <
                  submits_per_queue_[queue_idx].back().dependency_level) {
          }
        }
      }
    }
  }



  /* Calculate Sufficient Synchronization Index Set (SSIS) */
  using SSIS          = std::array<SyncNodeIndex, kMaxQueuesSupported>;
  std::vector<SSIS>       ssis(dag_.Size());




  // FIXME (tralf-strues): currently the time complexity is theoretically atrocious O(n^3),
  // yet there aren't that many nodes practically and we're only doing this once...
  for (int32_t dependency_level = max_dependency_level_; dependency_level >= 0; --dependency_level) {
    for (const auto& node : dag_) {
      if (GetDependencyLevel(dag_.GetNodeHandle(node)) != dependency_level) {
        continue;
      }

      auto& vulkan_node = GetVulkanNode(node);
      nodes_per_queue_[vulkan_node.queue_idx].push_back(&vulkan_node);

      // Find any nodes on other queues which depend on this one.
      for (uint32_t queue_idx = 0; queue_idx < queue_count; ++queue_idx) {
        if (vulkan_node.queue_idx == queue_idx) {
          continue;
        }

        for (auto other_node : nodes_per_queue_[queue_idx]) {
          if (dag_.EdgeExists(GetNodeHandle(vulkan_node), GetNodeHandle(*other_node)) &&
              submits_per_queue_[other_node->queue_idx].back().dependency_level <
                  submits_per_queue_[queue_idx].back().dependency_level) {
          }
        }
      }
    }
  }
}

VulkanRenderGraph::VulkanNode& VulkanRenderGraph::GetVulkanNode(const Node& node) {
  return GetVulkanNode(dag_.GetNodeHandle(node));
}

VulkanRenderGraph::VulkanNode& VulkanRenderGraph::GetVulkanNode(DAG<Node>::NodeHandle node_handle) {
  return vulkan_nodes_[node_handle];
}

DAG<VulkanRenderGraph::Node>::NodeHandle VulkanRenderGraph::GetNodeHandle(const VulkanNode& vulkan_node) {
  return static_cast<DAG<Node>::NodeHandle>(&vulkan_node - vulkan_nodes_.data());
}

}  // namespace liger::rhi