/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanRenderGraph.cpp
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

#include "VulkanRenderGraph.hpp"

#include <Liger-Engine/Core/EnumReflection.hpp>
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"

#include <fmt/ostream.h>

#include <fstream>
#include <unordered_map>

namespace liger::rhi {

void VulkanRenderGraph::ReimportTexture(ResourceVersion version, TextureResource new_texture) {
  resource_version_registry_.UpdateResource(resource_version_registry_.GetResourceId(version), new_texture);
  dirty_ = true;
}

void VulkanRenderGraph::ReimportBuffer(ResourceVersion version, BufferResource new_buffer) {
  resource_version_registry_.UpdateResource(resource_version_registry_.GetResourceId(version), new_buffer);
//   dirty_ = true; TODO(tralf-strues): dependent buffer info
}

void VulkanRenderGraph::UpdateTransientTextureSamples(ResourceVersion version, uint8_t new_sample_count) {
  auto& texture_info = transient_texture_infos_[resource_version_registry_.GetResourceId(version)];
  if (texture_info.samples.Get() != new_sample_count) {
    force_recreate_resources_ = true;
    texture_info.samples = new_sample_count;
  }
}

void VulkanRenderGraph::UpdateTransientBufferSize(ResourceVersion version, uint64_t new_size) {
  auto& buffer_info = transient_buffer_infos_[resource_version_registry_.GetResourceId(version)];
  if (buffer_info.size != new_size) {
    force_recreate_resources_ = true;
    buffer_info.size = new_size;
  }
}

void VulkanRenderGraph::Execute(Context& context, VkSemaphore wait, uint64_t wait_value, VkSemaphore signal, uint64_t signal_value) {
  if (first_frame_) {
    UpdateDependentResourceValues();
    RecreateTransientResources();
    SetupAttachments();
    LinkBarriersToResources();

    first_frame_ = false;
    dirty_ = false;
  }

  if (dirty_) {
    if (UpdateDependentResourceValues()) {
      RecreateTransientResources();
    }

    SetupAttachments();
    LinkBarriersToResources();
  }

  dirty_                    = false;
  force_recreate_resources_ = false;

  auto frame_idx = device_->CurrentFrame();
  command_pool_.Reset(frame_idx);

  auto submit = [&](uint32_t queue_idx, auto& submit_it, auto& cmds) {
    cmds->End();

    std::vector<VkSemaphoreSubmitInfo> wait_semaphores;
    std::vector<VkSemaphoreSubmitInfo> signal_semaphores;

    for (uint32_t wait_queue_idx = 0; wait_queue_idx < queue_count_; ++wait_queue_idx) {
      auto wait_info = submit_it->wait_per_queue[wait_queue_idx];

      if (wait_info.base_value != 0) {
        wait_semaphores.emplace_back(VkSemaphoreSubmitInfo {
          .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
          .pNext       = nullptr,
          .semaphore   = semaphores_per_queue_[wait_queue_idx].Get(),
          .value       = GetSemaphoreValue(wait_queue_idx, wait_info.base_value),
          .stageMask   = wait_info.stages,
          .deviceIndex = 0
        });
      }
    }

    if (wait != VK_NULL_HANDLE && queue_idx == 0 && submit_it == submits_per_queue_[queue_idx].begin()) {
      wait_semaphores.emplace_back(VkSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = wait,
        .value       = wait_value,
        .stageMask   = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .deviceIndex = 0
      });
    }

    if (submit_it->signal.base_value != 0) {
      signal_semaphores.emplace_back(VkSemaphoreSubmitInfo {
          .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
          .pNext       = nullptr,
          .semaphore   = semaphores_per_queue_[queue_idx].Get(),
          .value       = GetSemaphoreValue(queue_idx, submit_it->signal.base_value),
          .stageMask   = submit_it->signal.stages,
          .deviceIndex = 0
      });
    }

    if (signal != VK_NULL_HANDLE && queue_idx == 0 && submit_it + 1 == submits_per_queue_[queue_idx].end()) {
      signal_semaphores.emplace_back(VkSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = signal,
        .value       = signal_value,
        .stageMask   = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .deviceIndex = 0
      });
    }

    const VkCommandBufferSubmitInfo cmds_submit_info {
      .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .pNext         = nullptr,
      .commandBuffer = cmds->Get(),
      .deviceMask    = 0
    };

    const VkSubmitInfo2 submit_info {
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext                    = nullptr,
      .flags                    = 0,
      .waitSemaphoreInfoCount   = static_cast<uint32_t>(wait_semaphores.size()),
      .pWaitSemaphoreInfos      = wait_semaphores.data(),
      .commandBufferInfoCount   = 1,
      .pCommandBufferInfos      = &cmds_submit_info,
      .signalSemaphoreInfoCount = static_cast<uint32_t>(signal_semaphores.size()),
      .pSignalSemaphoreInfos    = signal_semaphores.data()
    };

    VULKAN_CALL(vkQueueSubmit2(vk_queues_[queue_idx], 1, &submit_info, VK_NULL_HANDLE));

    ++submit_it;
  };

  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    auto submit_it = submits_per_queue_[queue_idx].begin();

    std::optional<VulkanCommandBuffer> cmds = std::nullopt;

    for (auto* node : nodes_per_queue_[queue_idx]) {
      if (node->dependency_level > submit_it->dependency_level) {
        submit(queue_idx, submit_it, cmds);
        cmds = std::nullopt;
      }

      if (!cmds) {
        cmds = command_pool_.AllocateCommandBuffer(frame_idx, queue_idx);
        cmds->Begin();
      }

      const auto& original_node = dag_.GetNode(GetNodeHandle(*node));
      cmds->BeginDebugLabelRegion(original_node.name, GetDebugLabelColor(original_node.type));

      if (node->in_image_barrier_count > 0 || node->in_buffer_barrier_count > 0) {
        const VkDependencyInfo dependency_info {
          .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
          .pNext                    = nullptr,
          .dependencyFlags          = 0,
          .memoryBarrierCount       = 0,
          .pMemoryBarriers          = nullptr,
          .bufferMemoryBarrierCount = node->in_buffer_barrier_count,
          .pBufferMemoryBarriers    = &vk_buffer_barriers_[node->in_buffer_barrier_begin_idx],
          .imageMemoryBarrierCount  = node->in_image_barrier_count,
          .pImageMemoryBarriers     = &vk_image_barriers_[node->in_image_barrier_begin_idx]
        };

        vkCmdPipelineBarrier2(cmds->Get(), &dependency_info);
      }

      SetBufferPackBarriers(cmds->Get(), *node);

      const auto* rendering_info = node->rendering_info;

      if (rendering_info) {
        vkCmdBeginRendering(cmds->Get(), rendering_info);

        const VkViewport viewport {
          .x        = 0.0f,
          .y        = static_cast<float>(rendering_info->renderArea.extent.height),
          .width    = static_cast<float>(rendering_info->renderArea.extent.width),
          .height   = -static_cast<float>(rendering_info->renderArea.extent.height),
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
        };

        const VkRect2D scissor {
          .offset = rendering_info->renderArea.offset,
          .extent = rendering_info->renderArea.extent
        };

        vkCmdSetViewport(cmds->Get(), 0, 1, &viewport);
        vkCmdSetScissor(cmds->Get(), 0, 1, &scissor);

        vkCmdSetRasterizationSamplesEXT(cmds->Get(), static_cast<VkSampleCountFlagBits>(node->samples));
      }

      auto& job = dag_.GetNode(GetNodeHandle(*node)).job;

      if (job) {
        job(*this, context, *cmds);
      }

      if (node->rendering_info) {
        vkCmdEndRendering(cmds->Get());
      }

      if (node->out_image_barrier_count > 0) {
        const VkDependencyInfo dependency_info {
          .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
          .pNext                    = nullptr,
          .dependencyFlags          = 0,
          .memoryBarrierCount       = 0,
          .pMemoryBarriers          = nullptr,
          .bufferMemoryBarrierCount = 0,
          .pBufferMemoryBarriers    = nullptr,
          .imageMemoryBarrierCount  = node->out_image_barrier_count,
          .pImageMemoryBarriers     = &vk_image_barriers_[node->out_image_barrier_begin_idx]
        };

        vkCmdPipelineBarrier2(cmds->Get(), &dependency_info);
      }

      cmds->EndDebugLabelRegion();
    }

    if (submit_it != submits_per_queue_[queue_idx].end()) {
      submit(queue_idx, submit_it, cmds);
    }
  }
}

void VulkanRenderGraph::Compile(IDevice& device) {
  device_ = static_cast<VulkanDevice*>(&device);

  vulkan_nodes_.resize(dag_.Size());

  ScheduleToQueues();
  SetupAttachments();
  SetupBarriers();
  CreateSemaphores();

  command_pool_.Init(*device_, device_->GetFramesInFlight(), device_->GetDescriptorManager().GetSet(),
                     device_->GetQueues(), device_->GetDebugEnabled());
}

bool VulkanRenderGraph::UpdateDependentResourceValues() {
  bool changed_any = false;

  for (ResourceId resource_id = 0; resource_id < resource_version_registry_.GetResourceCount(); ++resource_id) {
    auto it = transient_texture_infos_.find(resource_id);
    if (it == transient_texture_infos_.end()) {
      continue;
    }

    DependentTextureInfo& dependent_info = it->second;
    bool changed = false;

    if (dependent_info.format.IsDependent()) {
      const auto dependency =
          resource_version_registry_.TryGetResourceByVersion<TextureResource>(dependent_info.format.GetDependency());

      if (dependency && dependency->texture) {
        changed = changed || (dependent_info.format.Get() != dependency->texture->GetInfo().format);
        dependent_info.format.UpdateDependentValue(dependency->texture->GetInfo().format);
      }
    }

    if (dependent_info.extent.IsDependent()) {
      const auto dependency =
          resource_version_registry_.TryGetResourceByVersion<TextureResource>(dependent_info.extent.GetDependency());

      if (dependency && dependency->texture) {
        changed = changed || (dependent_info.extent.Get() != dependency->texture->GetInfo().extent);
        dependent_info.extent.UpdateDependentValue(dependency->texture->GetInfo().extent);
      }
    }

    if (dependent_info.mip_levels.IsDependent()) {
      const auto dependency = resource_version_registry_.TryGetResourceByVersion<TextureResource>(
          dependent_info.mip_levels.GetDependency());

      if (dependency && dependency->texture) {
        changed = changed || (dependent_info.mip_levels.Get() != dependency->texture->GetInfo().mip_levels);
        dependent_info.mip_levels.UpdateDependentValue(dependency->texture->GetInfo().mip_levels);
      }
    }

    if (dependent_info.samples.IsDependent()) {
      const auto dependency =
          resource_version_registry_.TryGetResourceByVersion<TextureResource>(dependent_info.samples.GetDependency());

      if (dependency && dependency->texture) {
        changed = changed || (dependent_info.samples.Get() != dependency->texture->GetInfo().samples);
        dependent_info.samples.UpdateDependentValue(dependency->texture->GetInfo().samples);
      }
    }

    if (changed || force_recreate_resources_) {
      transient_textures_[resource_id] = device_->CreateTexture(dependent_info.Get());
      auto& texture = transient_textures_[resource_id];

      for (const auto& view_info : transient_texture_view_infos_[resource_id]) {
        texture->CreateView(view_info);
      }

      resource_version_registry_.UpdateResource(resource_id, TextureResource{texture.get(), kTextureDefaultViewIdx});
    }

    changed_any = changed_any || changed;
  }

  return changed_any;
}

void VulkanRenderGraph::RecreateTransientResources() {
  for (const auto& [id, info] : transient_buffer_infos_) {
    transient_buffers_[id] = device_->CreateBuffer(info);
    auto& buffer = transient_buffers_[id];
    resource_version_registry_.UpdateResource(id, buffer.get());
  }
}

void VulkanRenderGraph::SetupAttachments() {
  constexpr size_t kInvalidIdx = std::numeric_limits<size_t>::max();

  size_t render_pass_count      = 0;
  size_t total_attachment_count = 0;
  CalculateRenderPassCount(render_pass_count, total_attachment_count);

  vk_rendering_infos_.clear();
  vk_attachments_.clear();

  vk_rendering_infos_.resize(render_pass_count);
  vk_attachments_.resize(total_attachment_count);

  size_t cur_rendering_idx  = 0U;
  size_t cur_attachment_idx = 0U;

  for (const auto& node : dag_) {
    if (node.type != JobType::RenderPass) {
      continue;
    }

    Extent2D render_area;
    uint32_t samples = 1U;

    /* First get all color attachments */
    size_t first_color_attachment_idx = kInvalidIdx;
    uint32_t color_attachment_count = 0;

    for (const auto& write : node.write) {
      auto resource = resource_version_registry_.TryGetResourceByVersion<TextureResource>(write.version);
      if (!resource || !resource->texture) {
        continue;
      }

      auto* texture = static_cast<VulkanTexture*>(resource->texture);

      const auto extent = texture->GetInfo().extent;
      render_area.x = extent.x;
      render_area.y = extent.y;

      samples = texture->GetInfo().samples;

      if (write.state == DeviceResourceState::ColorTarget) {
        vk_attachments_[cur_attachment_idx] = VkRenderingAttachmentInfo {
          .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext              = nullptr,
          .imageView          = texture->GetVulkanView(resource->view),
          .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .resolveMode        = VK_RESOLVE_MODE_NONE,
          .resolveImageView   = VK_NULL_HANDLE,
          .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .loadOp             = GetVulkanAttachmentLoadOp(write.attachment_load),
          .storeOp            = GetVulkanAttachmentStoreOp(write.attachment_store),
          .clearValue         = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}}}
        };

        if (first_color_attachment_idx == kInvalidIdx) {
          first_color_attachment_idx = cur_attachment_idx;
        }

        ++cur_attachment_idx;
        ++color_attachment_count;
      }
    }

    /* Set color resolve */
    uint32_t color_resolve_count = 0;
    if (color_attachment_count > 0) {
      for (const auto& write : node.write) {
        auto resource = resource_version_registry_.TryGetResourceByVersion<TextureResource>(write.version);
        if (!resource || !resource->texture) {
          continue;
        }

        auto* texture = static_cast<VulkanTexture*>(resource->texture);

        if (write.state == DeviceResourceState::ColorMultisampleResolve) {
          auto& attachment_info = vk_attachments_[first_color_attachment_idx + color_resolve_count];
          attachment_info.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
          attachment_info.resolveImageView   = texture->GetVulkanView(resource->view);
          attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

          ++color_resolve_count;
        }
      }
    }

    /* Get depth stencil attachment */
    size_t depth_stencil_attachment_idx = kInvalidIdx;

    for (const auto& write : node.write) {
      auto resource = resource_version_registry_.TryGetResourceByVersion<TextureResource>(write.version);
      if (!resource || !resource->texture) {
        continue;
      }

      auto* texture = static_cast<VulkanTexture*>(resource->texture);

      if (write.state == DeviceResourceState::DepthStencilTarget) {
        vk_attachments_[cur_attachment_idx] = VkRenderingAttachmentInfo {
          .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext              = nullptr,
          .imageView          = texture->GetVulkanView(resource->view),
          .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .resolveMode        = VK_RESOLVE_MODE_NONE,
          .resolveImageView   = VK_NULL_HANDLE,
          .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .loadOp             = GetVulkanAttachmentLoadOp(write.attachment_load),
          .storeOp            = GetVulkanAttachmentStoreOp(write.attachment_store),
          .clearValue         = {.depthStencil = {.depth = 1.0f, .stencil = 0}}
        };

        if (depth_stencil_attachment_idx == kInvalidIdx) {
          depth_stencil_attachment_idx = cur_attachment_idx++;
        } else {
          LIGER_LOG_ERROR(kLogChannelRHI, "There cannot be two depth stencil attachments!");
          break;
        }
      }
    }

    /* Add rendering info */
    vk_rendering_infos_[cur_rendering_idx] = VkRenderingInfo {
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext                = nullptr,
      .flags                = 0,
      .renderArea           = VkRect2D{.offset = {0, 0}, .extent = {.width = render_area.x, .height = render_area.y}},
      .layerCount           = 1,
      .viewMask             = 0,
      .colorAttachmentCount = color_attachment_count,
      .pColorAttachments    = (first_color_attachment_idx != kInvalidIdx) ? &vk_attachments_[first_color_attachment_idx] : nullptr,
      .pDepthAttachment     = (depth_stencil_attachment_idx != kInvalidIdx) ? &vk_attachments_[depth_stencil_attachment_idx] : nullptr,
      .pStencilAttachment   = nullptr // TODO (tralf-strues): Add stencil attachments
    };

    GetVulkanNode(node).rendering_info = &vk_rendering_infos_[cur_rendering_idx];
    GetVulkanNode(node).samples        = samples;
    ++cur_rendering_idx;
  }
}

void VulkanRenderGraph::CalculateRenderPassCount(size_t& render_pass_count, size_t& total_attachment_count) const {
  for (const auto& node : dag_) {
    if (node.type != JobType::RenderPass) {
      continue;
    }

    ++render_pass_count;

    for (const auto& write : node.write) {
      if (write.state == DeviceResourceState::ColorTarget ||
          write.state == DeviceResourceState::ColorMultisampleResolve ||
          write.state == DeviceResourceState::DepthStencilTarget) {
        ++total_attachment_count;
      }
    }
  }
}

using SyncNodeIndex = uint32_t;

constexpr SyncNodeIndex CalculateSyncIndex(uint32_t sort_idx, uint32_t queue_idx, uint32_t nodes_count) {
  return sort_idx + queue_idx * nodes_count + 1;
}

void VulkanRenderGraph::ScheduleToQueues() {
  queue_count_ = 0;

  const uint32_t nodes_count = dag_.Size();
  const uint32_t main_queue_idx = queue_count_++;
  const uint32_t compute_queue_idx = (device_->GetQueues().GetComputeQueue()) ? (queue_count_++) : main_queue_idx;
  const uint32_t transfer_queue_idx = (device_->GetQueues().GetTransferQueue() ) ? (queue_count_++) : main_queue_idx;

  /* Set vk_queues_ */
  vk_queues_[main_queue_idx] = device_->GetQueues().GetMainQueue();

  if (auto vk_queue = device_->GetQueues().GetComputeQueue(); vk_queue) {
    vk_queues_[compute_queue_idx] = *vk_queue;
  }

  if (auto vk_queue = device_->GetQueues().GetTransferQueue(); vk_queue) {
    vk_queues_[transfer_queue_idx] = *vk_queue;
  }

  /* Set each Vulkan nodes' queue index */
  for (const auto& node : dag_) {
    auto& vulkan_node = GetVulkanNode(node);
    vulkan_node.name = node.name;

    vulkan_node.queue_idx = main_queue_idx;

    if (node.type == JobType::Compute && node.async) {
      vulkan_node.queue_idx = compute_queue_idx;
    }

    if (node.type == JobType::Transfer && node.async) {
      vulkan_node.queue_idx = transfer_queue_idx;
    }

    vulkan_node.dependency_level = GetDependencyLevel(dag_.GetNodeHandle(node));
  }

  /* Build reverse graph */
  auto reverse_dag = dag_.Reverse();

  /* Order nodes monotonically through dependency levels, needed later to construct SSIS. */
  std::vector<SyncNodeIndex>                    sync_index_from_handle(nodes_count);
  std::unordered_map<SyncNodeIndex, NodeHandle> handle_from_sync_index(nodes_count);

  for (uint32_t sort_idx = 0; sort_idx < nodes_count; ++sort_idx) {
    auto  node_handle = sorted_nodes_[sort_idx];
    auto* vulkan_node = &GetVulkanNode(node_handle);

    SyncNodeIndex sync_index            = CalculateSyncIndex(sort_idx, vulkan_node->queue_idx, nodes_count);
    handle_from_sync_index[sync_index]  = node_handle;
    sync_index_from_handle[node_handle] = sync_index;

    nodes_per_queue_[vulkan_node->queue_idx].push_back(vulkan_node);
  }

  /* Calculate Sufficient Synchronization Index Set (SSIS) and Covered mask */
  using SSIS        = std::array<SyncNodeIndex, kMaxQueuesSupported>;
  using CoveredMask = std::array<bool, kMaxQueuesSupported>;

  SSIS default_ssis;
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    default_ssis[queue_idx] = 0;
  }

  CoveredMask true_covered;
  CoveredMask false_covered;
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    true_covered[queue_idx]  = true;
    false_covered[queue_idx] = false;
  }

  std::vector<SSIS>        ssis_per_node(nodes_count, default_ssis);
  std::vector<CoveredMask> covered_per_node(nodes_count, true_covered);

  for (uint32_t sort_idx = 0; sort_idx < nodes_count; ++sort_idx) {
    auto node_handle = sorted_nodes_[sort_idx];
    auto queue_idx   = GetVulkanNode(node_handle).queue_idx;
    auto sync_idx    = CalculateSyncIndex(sort_idx, queue_idx, nodes_count);

    ssis_per_node[node_handle][queue_idx] = sync_idx;

    for (auto dependency_handle : reverse_dag.GetAdjacencyList(node_handle)) {
      auto dependency_queue_idx = GetVulkanNode(dependency_handle).queue_idx;

      if (dependency_queue_idx != queue_idx) {
        ssis_per_node[node_handle][dependency_queue_idx] = std::max(ssis_per_node[node_handle][dependency_queue_idx],
                                                                    sync_index_from_handle[dependency_handle]);
        covered_per_node[node_handle][dependency_queue_idx] = false;
      }
    }
  }

  /* Construct a new dependency graph without redundant dependencies */
  DAG<void> cross_queue_dependency_graph(nodes_count);

  bool covered_all = false;
  while (!covered_all) {
    covered_all = true;

    for (const auto& node : dag_) {
      auto node_handle = dag_.GetNodeHandle(node);
      auto& covered_final = covered_per_node[node_handle];

      std::optional<NodeHandle> best_dependency = std::nullopt;

      CoveredMask best_cover       = covered_final;
      uint32_t    best_cover_score = 0;

      for (auto dependency_handle : reverse_dag.GetAdjacencyList(node_handle)) {
        CoveredMask cover       = covered_final;
        uint32_t    cover_score = 0;

        for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
          if (!cover[queue_idx]) {
            cover[queue_idx] = ssis_per_node[dependency_handle][queue_idx] <= ssis_per_node[node_handle][queue_idx];

            if (cover[queue_idx]) {
              ++cover_score;
            }
          }
        }

        if (cover_score > best_cover_score ||
            (best_dependency.has_value() && cover_score == best_cover_score &&
             sync_index_from_handle[dependency_handle] > sync_index_from_handle[best_dependency.value()])) {
          best_dependency = dependency_handle;
          best_cover      = cover;
        }
      }

      if (best_dependency.has_value()) {
        covered_final = best_cover;
        cross_queue_dependency_graph.AddEdge(best_dependency.value(), node_handle);
      }

      if (std::find(covered_final.begin(), covered_final.begin() + queue_count_, false) !=
          covered_final.begin() + queue_count_) {
        covered_all = false;
      }
    }
  }

  /* Add submits and fill their dependency levels */
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    for (auto* vulkan_node : nodes_per_queue_[queue_idx]) {
      auto node_handle      = GetNodeHandle(*vulkan_node);
      auto dependency_level = GetDependencyLevel(node_handle);

      if (vulkan_node != nodes_per_queue_[queue_idx].back() &&
          cross_queue_dependency_graph.GetAdjacencyList(node_handle).empty()) {
        continue;
      }

      if (submits_per_queue_[queue_idx].empty() ||
          submits_per_queue_[queue_idx].back().dependency_level < dependency_level) {
        submits_per_queue_[queue_idx].emplace_back(Submit{.dependency_level = dependency_level});
      }
    }
  }

  /* Finally can set submits and semaphore synchronization */
  for (uint32_t sort_idx = 0; sort_idx < nodes_count; ++sort_idx) {
    auto node_handle      = sorted_nodes_[sort_idx];
    auto dependency_level = GetDependencyLevel(node_handle);
    auto queue_idx        = GetVulkanNode(node_handle).queue_idx;

    uint64_t submit_idx = 0;
    for (uint64_t i = 0; i < submits_per_queue_[queue_idx].size(); ++i) {
      if (submits_per_queue_[queue_idx][i].dependency_level >= dependency_level) {
        submit_idx = i;
        break;
      }
    }

    for (auto dependent_handle : dag_.GetAdjacencyList(node_handle)) {
      auto dependent_queue_idx = GetVulkanNode(dependent_handle).queue_idx;
      auto dependent_dependency_level = GetDependencyLevel(dependent_handle);

      uint64_t dependent_submit_idx = 0;
      for (uint64_t i = 0; i < submits_per_queue_[dependent_queue_idx].size(); ++i) {
        if (submits_per_queue_[dependent_queue_idx][i].dependency_level >= dependent_dependency_level) {
          dependent_submit_idx = i;
          break;
        }
      }

      if (queue_idx != dependent_queue_idx || dependent_submit_idx > submit_idx) {
        // Add wait on dependent submit
        auto& dependent_submit = submits_per_queue_[dependent_queue_idx][dependent_submit_idx];
        dependent_submit.wait_per_queue[queue_idx].base_value =
            std::max(dependent_submit.wait_per_queue[queue_idx].base_value, submit_idx + 1);

        // Add signal on this submit
        auto& submit = submits_per_queue_[queue_idx][submit_idx];
        submit.signal.base_value = submit_idx + 1;
      }
    }
  }

  // Additionally add signal values for the last submits
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    if (submits_per_queue_[queue_idx].empty()) {
      continue;
    }

    submits_per_queue_[queue_idx].back().signal.base_value = submits_per_queue_[queue_idx].size();
  }
}

void VulkanRenderGraph::SetupBarriers() {
  struct Usage {
    VkImageLayout             image_layout;
    VkAccessFlags2            access;
    VkPipelineStageFlags2     stages;
    std::optional<NodeHandle> node_handle{std::nullopt};
  };

  std::unordered_map<ResourceId, Usage> last_usages;

  for (auto [id, usage] : imported_resource_usages_) {
    if (!resource_version_registry_.TryGetResourceById<TextureResource>(id)) {
      continue;
    }

    last_usages[id] = Usage {
      .image_layout = GetVulkanImageLayout(usage.initial),
      .access       = GetVulkanAccessFlags(usage.initial),
      .stages       = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
    };
  }

  std::array<uint32_t, kMaxQueuesSupported> cur_submit_idx;
  std::fill(cur_submit_idx.begin(), cur_submit_idx.begin() + queue_count_, 0);

  std::array<uint32_t, kMaxQueuesSupported> cur_node_idx;
  std::fill(cur_node_idx.begin(), cur_node_idx.begin() + queue_count_, 0);

  bool work_left = true;

  while (work_left) {
    work_left = false;

    uint32_t first_queue = 0;
    for (uint32_t queue = 0; queue < queue_count_; ++queue) {
      if (cur_submit_idx[queue] >= submits_per_queue_[queue].size()) {
        continue;
      }

      work_left = true;

      if (cur_submit_idx[first_queue] >= submits_per_queue_[first_queue].size()) {
        first_queue = queue;
        continue;
      }

      if (submits_per_queue_[queue][cur_submit_idx[queue]].dependency_level <
          submits_per_queue_[first_queue][cur_submit_idx[first_queue]].dependency_level) {
        first_queue = queue;
      }
    }

    if (!work_left) {
      break;
    }

    ++cur_submit_idx[first_queue];

    for (; cur_node_idx[first_queue] < nodes_per_queue_[first_queue].size(); ++cur_node_idx[first_queue]) {
      auto&       vulkan_node = *nodes_per_queue_[first_queue][cur_node_idx[first_queue]];
      auto        node_handle = GetNodeHandle(vulkan_node);
      const auto& node        = dag_.GetNode(node_handle);

      auto add_in_image_barrier = [&](auto read_write) {
        const auto resource_id = resource_version_registry_.GetResourceId(read_write.version);
        if (!resource_version_registry_.TryGetResourceById<TextureResource>(resource_id)) {
          return;
        }

        const auto state    = read_write.state;
        const auto usage_it = last_usages.find(resource_id);

        VkImageMemoryBarrier2 image_barrier {
          .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .pNext               = nullptr,
          .srcStageMask        = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
          .srcAccessMask       = VK_ACCESS_2_NONE,
          .dstStageMask        = GetVulkanPipelineDstStage(node.type, state),
          .dstAccessMask       = GetVulkanAccessFlags(state),
          .oldLayout           = GetVulkanImageLayout(state),
          .newLayout           = GetVulkanImageLayout(state),
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image               = VK_NULL_HANDLE,  // Set inside LinkBarriersToResources
          .subresourceRange    = {}               // Set inside LinkBarriersToResources
        };

        if (usage_it != last_usages.end() && (usage_it->second.image_layout == image_barrier.newLayout)) {
          return;
        }

        const auto import_usage_it = imported_resource_usages_.find(resource_id);
        const auto usage_span = resource_usage_span_[resource_id];

        const bool is_first_node = (usage_span.first_node && usage_span.first_node == node_handle);
        const bool is_imported   = (import_usage_it != imported_resource_usages_.end());

        if (is_first_node && is_imported) {
          image_barrier.srcStageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
          image_barrier.srcAccessMask = GetVulkanAccessFlags(import_usage_it->second.initial);
          image_barrier.oldLayout     = GetVulkanImageLayout(import_usage_it->second.initial);
        } else if (is_first_node) {
          image_barrier.srcStageMask =
              GetVulkanPipelineSrcStage(dag_.GetNode(*usage_span.last_node).type, usage_span.last_state);
          image_barrier.srcAccessMask = GetVulkanAccessFlags(usage_span.last_state);
          image_barrier.oldLayout     = GetVulkanImageLayout(usage_span.last_state);
        }

        if (usage_it != last_usages.end()) {
          image_barrier.srcStageMask  = usage_it->second.stages;
          image_barrier.srcAccessMask = usage_it->second.access;
          image_barrier.oldLayout     = usage_it->second.image_layout;
        }

        last_usages[resource_id] = {
          .image_layout = image_barrier.newLayout,
          .access       = image_barrier.dstAccessMask,
          .stages       = image_barrier.dstStageMask
        };

        if (vulkan_node.in_image_barrier_count == 0) {
          vulkan_node.in_image_barrier_begin_idx = vk_image_barriers_.size();
        }

        vk_image_barriers_.emplace_back(image_barrier);
        image_barrier_resources_.emplace_back(resource_id);
        ++vulkan_node.in_image_barrier_count;
      };

      auto add_in_buffer_barrier = [&](auto read_write) {
        const auto resource_id = resource_version_registry_.GetResourceId(read_write.version);
        if (!resource_version_registry_.TryGetResourceById<BufferResource>(resource_id)) {
          return;
        }

        const auto state    = read_write.state;
        const auto usage_it = last_usages.find(resource_id);
        if (usage_it == last_usages.end()) {
          last_usages[resource_id] = {
            .access      = GetVulkanAccessFlags(state),
            .stages      = GetVulkanPipelineDstStage(node.type, state),
            .node_handle = node_handle
          };
          return;
        }

        auto& last_usage = usage_it->second;

        if (last_usage.node_handle && last_usage.node_handle == node_handle) {
          return;
        }

        auto dst_stages = GetVulkanPipelineDstStage(node.type, state);
        auto dst_access = GetVulkanAccessFlags(state);

        if (last_usage.access == dst_access) {
          return;
        }

        VkBufferMemoryBarrier2 buffer_barrier {
          .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
          .pNext               = nullptr,
          .srcStageMask        = last_usage.stages,
          .srcAccessMask       = last_usage.access,
          .dstStageMask        = dst_stages,
          .dstAccessMask       = dst_access,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .buffer              = VK_NULL_HANDLE,  // Set inside LinkBarriersToResources
          .offset              = 0U,              // Set inside LinkBarriersToResources
          .size                = 0U,              // Set inside LinkBarriersToResources
        };

        last_usages[resource_id] = {
          .access      = dst_access,
          .stages      = dst_stages,
          .node_handle = node_handle
        };

        if (vulkan_node.in_buffer_barrier_count == 0) {
          vulkan_node.in_buffer_barrier_begin_idx = vk_buffer_barriers_.size();
        }

        vk_buffer_barriers_.emplace_back(buffer_barrier);
        buffer_barrier_resources_.emplace_back(resource_id);
        ++vulkan_node.in_buffer_barrier_count;
      };

      auto add_in_buffer_pack_barrier = [&](auto read_write) {
        const auto resource_id = resource_version_registry_.GetResourceId(read_write.version);
        if (!resource_version_registry_.TryGetResourceById<BufferPackResource>(resource_id)) {
          return;
        }

        const auto state    = read_write.state;
        const auto usage_it = last_usages.find(resource_id);
        if (usage_it == last_usages.end()) {
          last_usages[resource_id] = {
            .access      = GetVulkanAccessFlags(state),
            .stages      = GetVulkanPipelineDstStage(node.type, state),
            .node_handle = node_handle
          };
          return;
        }

        auto& last_usage = usage_it->second;

        if (last_usage.node_handle && last_usage.node_handle == node_handle) {
          return;
        }

        auto dst_stages = GetVulkanPipelineDstStage(node.type, state);
        auto dst_access = GetVulkanAccessFlags(state);

        //if (last_usage.access == dst_access) {
        //  return;
        //}

        VkBufferMemoryBarrier2 buffer_barrier {
          .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
          .pNext               = nullptr,
          .srcStageMask        = last_usage.stages,
          .srcAccessMask       = last_usage.access,
          .dstStageMask        = dst_stages,
          .dstAccessMask       = dst_access,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .buffer              = VK_NULL_HANDLE,  // Set dynamically
          .offset              = 0U,              // Set dynamically
          .size                = 0U,              // Set dynamically
        };

        last_usages[resource_id] = {
          .access      = dst_access,
          .stages      = dst_stages,
          .node_handle = node_handle
        };

        if (vulkan_node.in_buffer_pack_barrier_count == 0) {
          vulkan_node.in_buffer_pack_barrier_begin_idx = vk_buffer_pack_barriers_.size();
        }

        vk_buffer_pack_barriers_.emplace_back(buffer_barrier);
        buffer_pack_barrier_resources_.emplace_back(resource_id);
        ++vulkan_node.in_buffer_pack_barrier_count;
      };

      for (const auto& read : node.read) {
        add_in_image_barrier(read);
        add_in_buffer_barrier(read);
        add_in_buffer_pack_barrier(read);
      }

      for (const auto& write : node.write) {
        add_in_image_barrier(write);
        add_in_buffer_barrier(write);
        add_in_buffer_pack_barrier(write);
      }

      for (const auto& write : node.write) {
        const auto resource_id = resource_version_registry_.GetResourceId(write.version);
        if (!resource_version_registry_.TryGetResourceById<TextureResource>(resource_id)) {
          continue;
        }

        const auto last_usage = last_usages[resource_id];

        auto import_usage_it = imported_resource_usages_.find(resource_id);
        if (import_usage_it == imported_resource_usages_.end() ||
            resource_usage_span_[resource_id].last_node != node_handle ||
            import_usage_it->second.final == DeviceResourceState::Undefined) {
          continue;
        }

        VkImageMemoryBarrier2 image_barrier {
          .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .pNext               = nullptr,
          .srcStageMask        = last_usage.stages,
          .srcAccessMask       = last_usage.access,
          .dstStageMask        = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
          .dstAccessMask       = GetVulkanAccessFlags(import_usage_it->second.final),
          .oldLayout           = last_usage.image_layout,
          .newLayout           = GetVulkanImageLayout(import_usage_it->second.final),
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image               = VK_NULL_HANDLE,  // Set inside LinkBarriersToResources
          .subresourceRange    = {}               // Set inside LinkBarriersToResources
        };

        if (vulkan_node.out_image_barrier_count == 0) {
          vulkan_node.out_image_barrier_begin_idx = vk_image_barriers_.size();
        }

        vk_image_barriers_.emplace_back(image_barrier);
        image_barrier_resources_.emplace_back(resource_id);
        ++vulkan_node.out_image_barrier_count;
      }
    }
  }
}

void VulkanRenderGraph::LinkBarriersToResources() {
  for (size_t barrier_idx = 0; barrier_idx < vk_image_barriers_.size(); ++barrier_idx) {
    auto& barrier  = vk_image_barriers_[barrier_idx];
    auto  resource = resource_version_registry_.GetResourceById<TextureResource>(image_barrier_resources_[barrier_idx]);

    const auto& view_info = resource.texture->GetViewInfo(resource.view);
    auto        format    = resource.texture->GetInfo().format;

    barrier.image = static_cast<const VulkanTexture*>(resource.texture)->GetVulkanImage();

    barrier.subresourceRange = {
      .aspectMask     = static_cast<VkImageAspectFlags>(IsDepthContainingFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
      .baseMipLevel   = view_info.first_mip,
      .levelCount     = view_info.mip_count,
      .baseArrayLayer = view_info.first_layer,
      .layerCount     = view_info.layer_count
    };
  }

  for (size_t barrier_idx = 0; barrier_idx < vk_buffer_barriers_.size(); ++barrier_idx) {
    auto& barrier  = vk_buffer_barriers_[barrier_idx];
    auto  resource = resource_version_registry_.GetResourceById<BufferResource>(buffer_barrier_resources_[barrier_idx]);

    barrier.buffer = static_cast<const VulkanBuffer*>(resource)->GetVulkanBuffer();
    barrier.offset = 0U;
    barrier.size   = resource->GetInfo().size;
  }
}

void VulkanRenderGraph::CreateSemaphores() {
  uint64_t max_time_point = 0;
  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    max_time_point = std::max<uint64_t>(max_time_point, submits_per_queue_[queue_idx].size() + 1);
  }

  for (uint32_t queue_idx = 0; queue_idx < queue_count_; ++queue_idx) {
    semaphores_per_queue_[queue_idx].Init(device_->GetVulkanDevice(), max_time_point);

    device_->SetDebugName(semaphores_per_queue_[queue_idx].Get(), "VulkanRenderGraph({0})::semaphores_per_queue_[{1}]",
                          name_, queue_idx);
  }
}

void VulkanRenderGraph::DumpGraphviz(std::string_view filename, bool detailed) {
  std::ofstream os(filename.data(), std::ios::out);
  if (!os.is_open()) {
    LIGER_LOG_ERROR(kLogChannelRHI, "Failed to open file '{0}'", filename);
    return;
  }

  fmt::print(os,
             "digraph {{\n"
             "labelloc=\"t\";\n"
             "label=\"VulkanRenderGraph ({0})\";\n"
             "fontname=\"helvetica\";\n"
             "fontsize=24;\n"
             "rankdir=LR;\n"
             "node [shape=record, fontname=\"helvetica\", fontsize=14, margin=\"0.2,0.15\"]\n\n",
             name_);

  constexpr uint32_t kFontSizeNode     = 16;
  constexpr uint32_t kFontSizeResource = 14;

  static const std::unordered_map<JobType, const char*> kFillcolorPerNodeType {
      {JobType::RenderPass, "goldenrod1"},
      {JobType::Compute,    "chartreuse3"},
      {JobType::Transfer,   "darkturquoise"}
  };

  constexpr const char* kFillcolorBuffer  = "gainsboro";
  constexpr const char* kFillcolorTexture = "slategray1";

  /* Dump nodes */
  for (DAG<int>::Depth d = 0; d <= max_dependency_level_; ++d) {
    fmt::print(os, "{{\nrank=same;\n");

    for (size_t sort_idx = 0; sort_idx < sorted_nodes_.size(); ++sort_idx) {
      auto        node_handle = sorted_nodes_[sort_idx];
      const auto& node        = dag_.GetNode(node_handle);
      const auto& vulkan_node = GetVulkanNode(node_handle);

      if (GetDependencyLevel(node_handle) == d) {
        fmt::print(os, "\tN{0} [shape=plaintext, label=<\n"
                       "\t\t<table border=\"3\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"5\">\n", node_handle);

        fmt::print(os, "\t\t\t<tr><td align=\"center\">");
        fmt::print(os, "<B>[{0}] {1}</B> <BR/><BR/> Dependency level: {2} {3}", sort_idx, node.name, d,
                   (vulkan_node.queue_idx != 0) ? "<BR/><BR/><U>Async</U>" : "");
        fmt::print(os, "</td></tr>\n");

        auto dump_image_barrier = [&](auto barrier_idx, auto type) {
          const auto texture =
              resource_version_registry_.GetResourceById<TextureResource>(image_barrier_resources_[barrier_idx]);
          const auto& barrier = vk_image_barriers_[barrier_idx];

          fmt::print(os, "\t\t\t<tr><td align=\"left\">");
          fmt::print(os, "[{0}] {1} barrier for <B>{2}</B> <BR align=\"left\"/><BR align=\"left\"/>", barrier_idx, type, texture.texture->GetInfo().name);
          fmt::print(os, "- srcStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.srcStageMask));
          fmt::print(os, "- srcAccessMask: {0} <BR align=\"left\"/>", string_VkAccessFlags2(barrier.srcAccessMask));
          fmt::print(os, "- oldLayout: {0} <BR align=\"left\"/><BR align=\"left\"/>", string_VkImageLayout(barrier.oldLayout));
          fmt::print(os, "- dstStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.dstStageMask));
          fmt::print(os, "- dstAccessMask: {0} <BR align=\"left\"/>", string_VkAccessFlags2(barrier.dstAccessMask));
          fmt::print(os, "- newLayout: {0}<BR align=\"left\"/>", string_VkImageLayout(barrier.newLayout));
          fmt::print(os, "</td></tr>\n");
        };

        auto dump_buffer_barrier = [&](auto barrier_idx, auto type) {
          const auto buffer =
              resource_version_registry_.GetResourceById<BufferResource>(buffer_barrier_resources_[barrier_idx]);
          const auto& barrier = vk_buffer_barriers_[barrier_idx];

          fmt::print(os, "\t\t\t<tr><td align=\"left\">");
          fmt::print(os, "[{0}] {1} barrier for <B>{2}</B> <BR align=\"left\"/><BR align=\"left\"/>", barrier_idx, type, buffer->GetInfo().name);
          fmt::print(os, "- srcStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.srcStageMask));
          fmt::print(os, "- srcAccessMask: {0} <BR align=\"left\"/><BR align=\"left\"/>", string_VkAccessFlags2(barrier.srcAccessMask));
          fmt::print(os, "- dstStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.dstStageMask));
          fmt::print(os, "- dstAccessMask: {0} <BR align=\"left\"/>", string_VkAccessFlags2(barrier.dstAccessMask));
          fmt::print(os, "</td></tr>\n");
        };

        auto dump_buffer_pack_barrier = [&](auto barrier_idx, auto type) {
          const auto  pack    = resource_version_registry_.GetResourceById<BufferPackResource>(buffer_pack_barrier_resources_[barrier_idx]);
          const auto& barrier = vk_buffer_pack_barriers_[barrier_idx];

          fmt::print(os, "\t\t\t<tr><td align=\"left\">");
          fmt::print(os, "[{0}] {1} barrier for <B>{2}</B> <BR align=\"left\"/><BR align=\"left\"/>", barrier_idx, type, pack.name);
          fmt::print(os, "- srcStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.srcStageMask));
          fmt::print(os, "- srcAccessMask: {0} <BR align=\"left\"/><BR align=\"left\"/>", string_VkAccessFlags2(barrier.srcAccessMask));
          fmt::print(os, "- dstStageMask: {0} <BR align=\"left\"/>", string_VkPipelineStageFlags2(barrier.dstStageMask));
          fmt::print(os, "- dstAccessMask: {0} <BR align=\"left\"/>", string_VkAccessFlags2(barrier.dstAccessMask));
          fmt::print(os, "</td></tr>\n");
        };

        if (detailed) {
          for (uint32_t i = 0; i < vulkan_node.in_image_barrier_count; ++i) {
            dump_image_barrier(vulkan_node.in_image_barrier_begin_idx + i, "In");
          }

          for (uint32_t i = 0; i < vulkan_node.in_buffer_barrier_count; ++i) {
            dump_buffer_barrier(vulkan_node.in_buffer_barrier_begin_idx + i, "In");
          }

          for (uint32_t i = 0; i < vulkan_node.in_buffer_pack_barrier_count; ++i) {
            dump_buffer_pack_barrier(vulkan_node.in_buffer_pack_barrier_begin_idx + i, "In");
          }

          for (uint32_t i = 0; i < vulkan_node.out_image_barrier_count; ++i) {
            dump_image_barrier(vulkan_node.out_image_barrier_begin_idx + i, "Out");
          }
        }

        fmt::print(os, "\t\t</table>\n");
        fmt::print(os, "\t> style=\"bold, filled\", fillcolor={0}, fontsize={1}, margin=\"0.0,0.0\"]\n",
                   kFillcolorPerNodeType.at(node.type), kFontSizeNode);
      }
    }

    fmt::print(os, "}}\n\n");
  }

  /* Dump resources */
  for (uint32_t version = 0; version < resource_version_registry_.GetVersionsCount(); ++version) {
    const auto buffer = resource_version_registry_.TryGetResourceByVersion<BufferResource>(version);
    if (buffer) {
      const auto& info = buffer.value()->GetInfo();

      if (detailed) {
        fmt::print(
            os,
            "R{0} "
            "[label=<{{ <B>{1}</B> <BR align=\"left\"/><BR align=\"left\"/> Size: {2} bytes <BR align=\"left\"/> "
            "Cpu visible: {3} <BR align=\"left\"/><BR align=\"left\"/> Usage: {4} <BR align=\"left\"/> | "
            "Version: {5} <BR/> ID: {6} }}> "
            "style=\"rounded, filled\", fillcolor={7}, fontsize={8}]\n",
            version, info.name, info.size, info.cpu_visible, EnumMaskToString(info.usage, ','), version,
            resource_version_registry_.GetResourceId(version), kFillcolorBuffer, kFontSizeResource);
      } else {
        fmt::print(os,
              "R{0} "
              "[label=<{{ <B>{1}</B> }}> "
              "style=\"rounded, filled\", fillcolor={2}, fontsize={3}]\n",
              version, info.name, kFillcolorBuffer, kFontSizeResource);
      }
    }

    const auto buffer_pack = resource_version_registry_.TryGetResourceByVersion<BufferPackResource>(version);
    if (buffer_pack) {
      size_t count = buffer_pack->buffers ? buffer_pack->buffers->size() : 0U;

      if (detailed) {
        fmt::print(os,
                   "R{0} "
                   "[label=<{{ <B>{1}</B> <BR/><BR/> "
                   "[Buffer Pack] <BR/> Buffers: {2} | "
                   "Version: {3} <BR/> ID: {4} }}> "
                   "style=\"dashed, rounded, filled\", fillcolor={5}, fontsize={6}]\n",
                   version, buffer_pack->name, count, version, resource_version_registry_.GetResourceId(version),
                   kFillcolorBuffer, kFontSizeResource);
      } else {
        fmt::print(os,
                   "R{0} "
                   "[label=<{{ <B>{1}</B> <BR/><BR/> "
                   "[Buffer Pack] <BR/> Buffers: {2} }}> "
                   "style=\"dashed, rounded, filled\", fillcolor={3}, fontsize={4}]\n",
                   version, buffer_pack->name, count, kFillcolorBuffer, kFontSizeResource);
      }
    }

    const auto texture = resource_version_registry_.TryGetResourceByVersion<TextureResource>(version);
    if (texture) {
      const auto& info = texture.value().texture->GetInfo();

      if (detailed) {
        fmt::print(os,
                   "R{0} [label=<{{ <B>{1}</B> <BR align=\"left\"/><BR align=\"left\"/>"
                   "Extent: {2} x {3} x {4} <BR align=\"left\"/>"
                   "Samples: {5} <BR align=\"left\"/>"
                   "Mip levels: {6} <BR align=\"left\"/>"
                   "Format: {7} <BR align=\"left\"/><BR align=\"left\"/>"
                   "Usage: {8} <BR align=\"left\"/> | "
                   "Version: {9} <BR/> ID: {10} <BR/><BR/> View: {11} }}> "
                   "style=\"rounded, filled\", fillcolor={12}, fontsize={13}]\n",
                   version, info.name, info.extent.x, info.extent.y, info.extent.z, info.samples, info.mip_levels,
                   EnumToString(info.format), EnumMaskToString(info.usage, ','), version,
                   resource_version_registry_.GetResourceId(version), texture->view, kFillcolorTexture,
                   kFontSizeResource);
      } else {
        fmt::print(os,
                   "R{0} [label=<{{ <B>{1}</B> }}> "
                   "style=\"rounded, filled\", fillcolor={2}, fontsize={3}]\n",
                   version, info.name, kFillcolorTexture, kFontSizeResource);
      }
    }
  }

  /* Edges */
  for (const auto& node : dag_) {
    auto node_handle = dag_.GetNodeHandle(node);

    for (const auto& read : node.read) {
      if (detailed) {
        fmt::print(os, "R{0} -> N{1} [label=\"{2}\", fontcolor=gray, color=gray]\n", read.version, node_handle,
                   EnumMaskToString(read.state));
      } else {
        fmt::print(os, "R{0} -> N{1} [fontcolor=gray, color=gray]\n", read.version, node_handle);
      }
    }

    for (const auto& write : node.write) {
      std::string store_str;
      if (write.state == DeviceResourceState::DepthStencilTarget || write.state == DeviceResourceState::ColorTarget) {
        store_str = fmt::format(", Store = {0}", EnumToString(write.attachment_store));
      }

      if (detailed) {
        fmt::print(os, "N{0} -> R{1} [label=\"{2}{3}\", fontcolor=black, color=black]\n", node_handle, write.version,
                   EnumMaskToString(write.state), store_str);
      } else {
        fmt::print(os, "N{0} -> R{1} [fontcolor=black, color=black]\n", node_handle, write.version);
      }
    }
  }

  fmt::print(os, "\n}}\n");
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

uint64_t VulkanRenderGraph::GetSemaphoreValue(uint32_t queue_idx, uint64_t base_value) const {
  return device_->CurrentAbsoluteFrame() * (submits_per_queue_[queue_idx].size() + 1) + base_value;
}

void VulkanRenderGraph::SetBufferPackBarriers(VkCommandBuffer vk_cmds, VulkanNode& vulkan_node) const {
  if (vulkan_node.in_buffer_pack_barrier_count == 0U) {
    return;
  }

  std::vector<VkBufferMemoryBarrier2> barriers;
  for (uint32_t i = 0U; i < vulkan_node.in_buffer_pack_barrier_count; ++i) {
    const size_t start_idx   = barriers.size();
    const size_t barrier_idx = vulkan_node.in_buffer_pack_barrier_begin_idx + i;

    auto pack =
      *resource_version_registry_.TryGetResourceById<BufferPackResource>(buffer_pack_barrier_resources_[barrier_idx]);
    barriers.insert(barriers.end(), pack.buffers->size(), vk_buffer_pack_barriers_[barrier_idx]);

    for (size_t buffer_idx = 0U; buffer_idx < pack.buffers->size(); ++buffer_idx) {
      auto& barrier = barriers[start_idx + buffer_idx];

      barrier.buffer = static_cast<const VulkanBuffer*>(pack.buffers->at(buffer_idx))->GetVulkanBuffer();
      barrier.offset = 0U;
      barrier.size   = VK_WHOLE_SIZE;
    }
  }

  const VkDependencyInfo dependency_info {
    .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .pNext                    = nullptr,
    .dependencyFlags          = 0,
    .memoryBarrierCount       = 0,
    .pMemoryBarriers          = nullptr,
    .bufferMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
    .pBufferMemoryBarriers    = barriers.data(),
    .imageMemoryBarrierCount  = 0,
    .pImageMemoryBarriers     = nullptr
  };

  vkCmdPipelineBarrier2(vk_cmds, &dependency_info);
}

glm::vec4 VulkanRenderGraph::GetDebugLabelColor(JobType node_type) {
  switch (node_type) {
    case JobType::RenderPass: { return glm::vec4(1.0f, 0.757f, 0.145f, 1.0f); }
    case JobType::Compute:    { return glm::vec4(0.4f, 0.804f, 0.0f,   1.0f);; }
    case JobType::Transfer:   { return glm::vec4(0.0f, 0.81f,  0.82f,  1.0f);; }

    default:                     { return glm::vec4(1.0f); }
  }

  return glm::vec4(1.0f);
}

}  // namespace liger::rhi