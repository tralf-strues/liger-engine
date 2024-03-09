/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file render_graph.hpp
 * @date 2023-12-11
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

#include <liger/core/dag.hpp>
#include <liger/render/rhi/buffer.hpp>
#include <liger/render/rhi/command_buffer.hpp>
#include <liger/render/rhi/dependent_texture_info.hpp>
#include <liger/render/rhi/resource_version_registry.hpp>

#include <optional>
#include <string_view>
#include <unordered_map>

namespace liger::rhi {

class IDevice;

enum class AttachmentLoad : uint8_t {
  kLoad,
  kClear,
  kDontCare
};

enum class AttachmentStore : uint8_t {
  kStore,
  kDiscard
};

class RenderGraph {
 public:
  struct TextureResource {
    ITexture* texture {nullptr};
    uint32_t  view    {kTextureDefaultViewIdx};
  };

  using BufferResource          = IBuffer*;
  using ResourceVersionRegistry = ResourceVersionRegistry<TextureResource, BufferResource>;
  using ResourceVersion         = ResourceVersionRegistry::ResourceVersion;
  using ResourceId              = ResourceVersionRegistry::ResourceId;
  using DependentTextureInfo    = DependentTextureInfo<ResourceVersion>;
  using Job = std::function<void(ICommandBuffer& cmds)>;

  virtual ~RenderGraph() = default;

  TextureResource GetTexture(ResourceVersion version);
  BufferResource GetBuffer(ResourceVersion version);

  virtual void ReimportTexture(ResourceVersion version, TextureResource new_texture) = 0;
  virtual void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) = 0;
  virtual void DumpGraphviz(std::string_view filename) = 0;

  void SetJob(std::string_view node_name, Job job);

 protected:
  struct ResourceRead {
    ResourceVersion     version;
    DeviceResourceState state;
  };

  struct ResourceWrite {
    ResourceVersion     version;
    DeviceResourceState state;

    AttachmentLoad  attachment_load;
    AttachmentStore attachment_store;
  };

  struct ImportedResourceUsage {
    DeviceResourceState initial = DeviceResourceState::kUndefined;
    DeviceResourceState final   = DeviceResourceState::kUndefined;
  };

  struct Node {
    enum class Type : uint8_t {
      kRenderPass,
      kCompute,
      kTransfer
    };

    Type                       type;
    ICommandBuffer::Capability command_capabilities;
    bool                       async;
    std::string                name;
    std::vector<ResourceRead>  read;
    std::vector<ResourceWrite> write;
    Job                        job;
  };

  using NodeGraph       = DAG<Node>;
  using NodeHandle      = NodeGraph::NodeHandle;
  using DependencyLevel = NodeGraph::Depth;

  struct ResourceUsageSpan {
    std::optional<NodeHandle> first_node  = std::nullopt;
    DeviceResourceState       first_state = DeviceResourceState::kUndefined;
    std::optional<NodeHandle> last_node   = std::nullopt;
    DeviceResourceState       last_state  = DeviceResourceState::kUndefined;
  };

  RenderGraph() = default;

  virtual void Compile(IDevice& device) = 0;

  NodeHandle GetSortedNode(uint32_t sorted_idx) const;
  DependencyLevel GetDependencyLevel(NodeHandle node_handle) const;

  std::string                                           name_;
  NodeGraph                                             dag_;
  NodeGraph::SortedList                                 sorted_nodes_;
  NodeGraph::DepthList                                  node_dependency_levels_;
  uint32_t                                              max_dependency_level_;
  ResourceVersionRegistry                               resource_version_registry_;
  std::unordered_map<ResourceId, DependentTextureInfo>  transient_texture_infos_;
  std::unordered_map<ResourceId, IBuffer::Info>         transient_buffer_infos_;
  std::unordered_map<ResourceId, ImportedResourceUsage> imported_resource_usages_;
  std::unordered_map<ResourceId, ResourceUsageSpan>     resource_usage_span_;

  friend class RenderGraphBuilder;
};

class RenderGraphBuilder {
 public:
  using ResourceVersion      = RenderGraph::ResourceVersion;
  using DependentTextureInfo = RenderGraph::DependentTextureInfo;

  explicit RenderGraphBuilder(std::unique_ptr<RenderGraph> graph);

  RenderGraphBuilder(const RenderGraphBuilder& other) = delete;
  RenderGraphBuilder& operator=(const RenderGraphBuilder& other) = delete;

  [[nodiscard]] ResourceVersion DeclareTransientTexture(const DependentTextureInfo& info);
  [[nodiscard]] ResourceVersion DeclareTransientBuffer(const IBuffer::Info& info);

  [[nodiscard]] ResourceVersion DeclareImportTexture(DeviceResourceState initial_state,
                                                     DeviceResourceState final_state);
  [[nodiscard]] ResourceVersion DeclareImportBuffer(DeviceResourceState initial_state,
                                                     DeviceResourceState final_state);

  void BeginRenderPass(std::string_view           name,
                       ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kGraphics);
  void EndRenderPass();

  void BeginCompute(std::string_view name, bool async = false,
                    ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kCompute);
  void EndCompute();

  void BeginTransfer(std::string_view name, bool async = false,
                     ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kTransfer);
  void EndTransfer();

  ResourceVersion AddColorTarget(ResourceVersion texture, AttachmentLoad load, AttachmentStore store);
  ResourceVersion SetDepthStencil(ResourceVersion texture, AttachmentLoad load, AttachmentStore store);
  void SampleTexture(ResourceVersion texture);

  void ReadBuffer(ResourceVersion buffer, DeviceResourceState usage);
  void WriteBuffer(ResourceVersion buffer, DeviceResourceState usage);

  [[nodiscard]] std::unique_ptr<RenderGraph> Build(IDevice& device, std::string_view name);

 private:
  void BeginNode(RenderGraph::Node::Type type, bool async, ICommandBuffer::Capability capabilities,
                 std::string_view name);
  void EndNode(RenderGraph::Node::Type type);

  RenderGraphBuilder::ResourceVersion AddWrite(RenderGraph::Node::Type type, ResourceVersion resource,
                                               DeviceResourceState usage);

  std::unique_ptr<RenderGraph> graph_;
  std::optional<RenderGraph::NodeHandle> current_node_ = std::nullopt;
};

}  // namespace liger::rhi