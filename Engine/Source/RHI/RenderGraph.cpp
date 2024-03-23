/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file RenderGraph.cpp
 * @date 2024-01-04
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

#include <Liger-Engine/RHI/RenderGraph.hpp>

#include <Liger-Engine/RHI/LogChannel.hpp>

namespace liger::rhi {

RenderGraph::TextureResource RenderGraph::GetTexture(ResourceVersion version) {
  return resource_version_registry_.GetResourceByVersion<TextureResource>(version);
}

RenderGraph::BufferResource RenderGraph::GetBuffer(ResourceVersion version) {
  return resource_version_registry_.GetResourceByVersion<BufferResource>(version);
}

void RenderGraph::SetJob(const std::string_view node_name, Job job) {
  for (auto& node : dag_) {
    if (node.name == node_name) {
      node.job = std::move(job);
    }
  }
}

DAG<RenderGraph::Node>::NodeHandle RenderGraph::GetSortedNode(uint32_t sorted_idx) const {
  return sorted_nodes_[sorted_idx];
}

DAG<RenderGraph::Node>::Depth RenderGraph::GetDependencyLevel(NodeHandle node_handle) const {
  return node_dependency_levels_[node_handle];
}

RenderGraphBuilder::RenderGraphBuilder(std::unique_ptr<RenderGraph> graph) : graph_(std::move(graph)) {}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::DeclareTransientTexture(const DependentTextureInfo& info) {
  auto version = graph_->resource_version_registry_.DeclareResource();
  graph_->transient_texture_infos_[graph_->resource_version_registry_.GetResourceId(version)] = info;
  return version;
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::DeclareTransientBuffer(const IBuffer::Info& info) {
  auto version = graph_->resource_version_registry_.DeclareResource();
  graph_->transient_buffer_infos_[graph_->resource_version_registry_.GetResourceId(version)] = info;
  return version;
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::DeclareImportTexture(DeviceResourceState initial_state,
                                                                             DeviceResourceState final_state) {
  auto version = graph_->resource_version_registry_.DeclareResource();
  graph_->imported_resource_usages_[graph_->resource_version_registry_.GetResourceId(version)] = {
      .initial = initial_state, .final = final_state};

  return version;
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::DeclareImportBuffer(DeviceResourceState initial_state,
                                                                            DeviceResourceState final_state) {
  auto version = graph_->resource_version_registry_.DeclareResource();
  graph_->imported_resource_usages_[graph_->resource_version_registry_.GetResourceId(version)] = {
      .initial = initial_state, .final = final_state};

  return version;
}

void RenderGraphBuilder::BeginRenderPass(std::string_view name, ICommandBuffer::Capability capabilities) {
  BeginNode(RenderGraph::Node::Type::RenderPass, false, capabilities, name);
}

void RenderGraphBuilder::EndRenderPass() {
  EndNode(RenderGraph::Node::Type::RenderPass);
}

void RenderGraphBuilder::BeginCompute(std::string_view name, bool async, ICommandBuffer::Capability capabilities) {
  BeginNode(RenderGraph::Node::Type::Compute, async, capabilities, name);
}

void RenderGraphBuilder::EndCompute() {
  EndNode(RenderGraph::Node::Type::Compute);
}

void RenderGraphBuilder::BeginTransfer(std::string_view name, bool async, ICommandBuffer::Capability capabilities) {
  BeginNode(RenderGraph::Node::Type::Transfer, async, capabilities, name);
}

void RenderGraphBuilder::EndTransfer() {
  EndNode(RenderGraph::Node::Type::Transfer);
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::AddColorTarget(ResourceVersion texture, AttachmentLoad load,
                                                                       AttachmentStore store) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  LIGER_ASSERT(node.type == RenderGraph::Node::Type::RenderPass, kLogChannelRHI,
               "Incompatible resource access with the current node type!");

  auto new_version = texture;

  if (load == AttachmentLoad::Load) {
    node.read.push_back(RenderGraph::ResourceRead {
      .version = new_version,
      .state   = DeviceResourceState::ColorTarget
    });

    new_version = graph_->resource_version_registry_.NextVersion(texture);
  }

  node.write.push_back(RenderGraph::ResourceWrite {
    .version          = new_version,
    .state            = DeviceResourceState::ColorTarget,
    .attachment_load  = load,
    .attachment_store = store
  });

  return new_version;
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::SetDepthStencil(ResourceVersion texture, AttachmentLoad load,
                                                                        AttachmentStore store) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  LIGER_ASSERT(node.type == RenderGraph::Node::Type::RenderPass, kLogChannelRHI,
               "Incompatible resource access with the current node type!");

  auto new_version = texture;

  if (load == AttachmentLoad::Load) {
    node.read.push_back(RenderGraph::ResourceRead {
      .version = new_version,
      .state   = DeviceResourceState::DepthStencilTarget
    });

    new_version = graph_->resource_version_registry_.NextVersion(texture);
  }

  node.write.push_back(RenderGraph::ResourceWrite {
    .version          = new_version,
    .state            = DeviceResourceState::DepthStencilTarget,
    .attachment_load  = load,
    .attachment_store = store
  });

  return new_version;
}

void RenderGraphBuilder::SampleTexture(ResourceVersion texture) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  auto usage = DeviceResourceState::ShaderSampled;

  node.read.push_back(RenderGraph::ResourceRead{texture, usage});
}

void RenderGraphBuilder::ReadBuffer(ResourceVersion buffer, DeviceResourceState usage) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  node.read.push_back(RenderGraph::ResourceRead{.version = buffer, .state = usage});
}

void RenderGraphBuilder::WriteBuffer(ResourceVersion buffer, DeviceResourceState usage) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  node.write.push_back(RenderGraph::ResourceWrite{.version = buffer, .state = usage});
}

std::unique_ptr<RenderGraph> RenderGraphBuilder::Build(IDevice& device, std::string_view name) {
  auto& dag     = graph_->dag_;
  graph_->name_ = name;

  for (auto& from : dag) {
    for (auto& to : dag) {
      for (auto from_write : from.write) {
        for (auto to_read : to.read) {
          if (from_write.version == to_read.version) {
            dag.AddEdge(from, to);
          }
        }
      }
    }
  }

  dag.TopologicalSort(graph_->sorted_nodes_, graph_->node_dependency_levels_, graph_->max_dependency_level_);

  auto add_usage = [&](auto node_handle, auto resource_id, auto state) {
    auto it           = graph_->resource_usage_span_.find(resource_id);
    auto exists       = (it != graph_->resource_usage_span_.end());
    auto exists_first = exists && it->second.first_node;
    auto exists_last  = exists && it->second.last_node;

    if (!exists) {
      graph_->resource_usage_span_[resource_id] = {};
    }

    if (!exists_first || graph_->GetDependencyLevel(node_handle) < graph_->GetDependencyLevel(*it->second.first_node)) {
      graph_->resource_usage_span_[resource_id].first_node  = node_handle;
      graph_->resource_usage_span_[resource_id].first_state = state;
    }

    if (!exists_last || graph_->GetDependencyLevel(node_handle) > graph_->GetDependencyLevel(*it->second.last_node)) {
      graph_->resource_usage_span_[resource_id].last_node  = node_handle;
      graph_->resource_usage_span_[resource_id].last_state = state;
    }
  };

  for (const auto& node : dag) {
    auto handle = dag.GetNodeHandle(node);

    for (auto read : node.read) {
      add_usage(handle, graph_->resource_version_registry_.GetResourceId(read.version), read.state);
    }

    for (auto write : node.write) {
      add_usage(handle, graph_->resource_version_registry_.GetResourceId(write.version), write.state);
    }
  }

  graph_->Compile(device);

  return std::move(graph_);
}

void RenderGraphBuilder::BeginNode(RenderGraph::Node::Type type, bool async, ICommandBuffer::Capability capabilities,
                                   const std::string_view name) {
  LIGER_ASSERT(!current_node_.has_value(), kLogChannelRHI,
               "Cannot begin a render graph node without ending the previous one!");

  RenderGraph::Node node;
  node.type                 = type;
  node.command_capabilities = capabilities;
  node.async                = async;
  node.name                 = name;
  current_node_             = graph_->dag_.EmplaceNode(std::move(node));
}

void RenderGraphBuilder::EndNode(RenderGraph::Node::Type type) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI,
               "Cannot end a render graph node without beginning it prior to this!");

  LIGER_ASSERT(graph_->dag_.GetNode(*current_node_).type == type, kLogChannelRHI,
               "End function type does not math the begin function type!");

  current_node_.reset();
}

RenderGraphBuilder::ResourceVersion RenderGraphBuilder::AddWrite(RenderGraph::Node::Type type, ResourceVersion resource,
                                                                 DeviceResourceState usage) {
  LIGER_ASSERT(current_node_.has_value(), kLogChannelRHI, "Adding resource access outside of begin/end scope!");

  auto& node = graph_->dag_.GetNode(*current_node_);
  LIGER_ASSERT(node.type == type, kLogChannelRHI, "Incompatible resource access with the current node type!");

  node.read.push_back(RenderGraph::ResourceRead{resource, usage});

  auto new_version = graph_->resource_version_registry_.NextVersion(resource);
  node.write.push_back(RenderGraph::ResourceWrite{.version = new_version, .state = usage});

  return new_version;
}

}  // namespace liger::rhi