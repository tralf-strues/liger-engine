/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file RenderGraph.hpp
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

#include <Liger-Engine/Core/Containers/DependencyGraph.hpp>
#include <Liger-Engine/RHI/Buffer.hpp>
#include <Liger-Engine/RHI/CommandBuffer.hpp>
#include <Liger-Engine/RHI/Context.hpp>
#include <Liger-Engine/RHI/DependentTextureInfo.hpp>
#include <Liger-Engine/RHI/ResourceVersionRegistry.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace liger::rhi {

class IDevice;

enum class AttachmentLoad : uint8_t {
  Load,
  Clear,
  DontCare
};

enum class AttachmentStore : uint8_t {
  Store,
  Discard
};

class RenderGraph {
 public:
  struct TextureResource {
    ITexture* texture {nullptr};
    uint32_t  view    {kTextureDefaultViewIdx};
  };

  using BufferResource = IBuffer*;

  struct BufferPackResource {
    std::string_view       name;
    std::vector<IBuffer*>* buffers;
  };

  using ResourceVersionRegistry = ResourceVersionRegistry<TextureResource, BufferResource, BufferPackResource>;
  using ResourceVersion         = ResourceVersionRegistry::ResourceVersion;
  using ResourceId              = ResourceVersionRegistry::ResourceId;
  using DependentTextureInfo    = DependentTextureInfo<ResourceVersion>;
  using Job                     = std::function<void(RenderGraph&, Context&, ICommandBuffer&)>;

  virtual ~RenderGraph() = default;

  TextureResource    GetTexture(ResourceVersion version);
  BufferResource     GetBuffer(ResourceVersion version);
  BufferPackResource GetBufferPack(ResourceVersion version);

  virtual void ReimportTexture(ResourceVersion version, TextureResource new_texture) = 0;
  virtual void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) = 0;
  virtual void UpdateTransientTextureSamples(ResourceVersion version, uint8_t new_sample_count) = 0;
  virtual void DumpGraphviz(std::string_view filename, bool detailed) = 0;

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
    DeviceResourceState initial = DeviceResourceState::Undefined;
    DeviceResourceState final   = DeviceResourceState::Undefined;
  };

  struct Node {
    JobType                    type;
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
  using TextureViewList = std::vector<rhi::TextureViewInfo>;

  struct ResourceUsageSpan {
    std::optional<NodeHandle> first_node  = std::nullopt;
    DeviceResourceState       first_state = DeviceResourceState::Undefined;
    std::optional<NodeHandle> last_node   = std::nullopt;
    DeviceResourceState       last_state  = DeviceResourceState::Undefined;
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
  std::unordered_map<ResourceId, TextureViewList>       transient_texture_view_infos_;
  std::unordered_map<ResourceId, IBuffer::Info>         transient_buffer_infos_;
  std::unordered_map<ResourceId, ImportedResourceUsage> imported_resource_usages_;
  std::unordered_map<ResourceId, ResourceUsageSpan>     resource_usage_span_;

  friend class RenderGraphBuilder;
};

class RenderGraphBuilder {
 public:
  using ResourceVersion      = RenderGraph::ResourceVersion;
  using DependentTextureInfo = RenderGraph::DependentTextureInfo;

  explicit RenderGraphBuilder(std::unique_ptr<RenderGraph> graph, Context& context);

  RenderGraphBuilder(const RenderGraphBuilder& other) = delete;
  RenderGraphBuilder& operator=(const RenderGraphBuilder& other) = delete;

  [[nodiscard]] ResourceVersion DeclareTransientTexture(const DependentTextureInfo& info);
  void DeclareTextureView(ResourceVersion texture, const rhi::TextureViewInfo& view_info);

  [[nodiscard]] ResourceVersion DeclareTransientBuffer(const IBuffer::Info& info);

  [[nodiscard]] ResourceVersion DeclareImportTexture(DeviceResourceState initial_state,
                                                     DeviceResourceState final_state);
  [[nodiscard]] ResourceVersion DeclareImportBuffer(DeviceResourceState initial_state,
                                                    DeviceResourceState final_state);
  [[nodiscard]] ResourceVersion DeclareImportBufferPack(std::string_view name,
                                                        DeviceResourceState initial_state,
                                                        DeviceResourceState final_state);

  [[nodiscard]] ResourceVersion ImportTexture(RenderGraph::TextureResource texture, DeviceResourceState initial_state,
                                              DeviceResourceState final_state);
  [[nodiscard]] ResourceVersion ImportBuffer(RenderGraph::BufferResource buffer, DeviceResourceState initial_state,
                                             DeviceResourceState final_state);

  [[nodiscard]] ResourceVersion LastResourceVersion(ResourceVersion resource);

  void BeginRenderPass(std::string_view           name,
                       ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::Graphics);
  void EndRenderPass();

  void BeginCompute(std::string_view name, bool async = false,
                    ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::Compute);
  void EndCompute();

  void BeginTransfer(std::string_view name, bool async = false,
                     ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::Transfer);
  void EndTransfer();

  void SetJob(RenderGraph::Job job);

  ResourceVersion AddColorTarget(ResourceVersion texture, AttachmentLoad load, AttachmentStore store);
  ResourceVersion AddColorMultisampleResolve(ResourceVersion texture);
  ResourceVersion SetDepthStencil(ResourceVersion texture, AttachmentLoad load, AttachmentStore store);

  void SampleTexture(ResourceVersion texture);
  void WriteTexture(ResourceVersion texture);
  ResourceVersion ReadWriteTexture(ResourceVersion texture);

  void ReadBuffer(ResourceVersion buffer, DeviceResourceState usage);
  void WriteBuffer(ResourceVersion buffer, DeviceResourceState usage);
  ResourceVersion ReadWriteBuffer(ResourceVersion buffer, DeviceResourceState usage);

  Context& GetContext();

  [[nodiscard]] std::unique_ptr<RenderGraph> Build(IDevice& device, std::string_view name);

 private:
  void BeginNode(JobType type, bool async, ICommandBuffer::Capability capabilities,
                 std::string_view name);
  void EndNode(JobType type);

  RenderGraphBuilder::ResourceVersion AddWrite(JobType type, ResourceVersion resource,
                                               DeviceResourceState usage);

  std::unique_ptr<RenderGraph>           graph_;
  Context&                               context_;
  std::optional<RenderGraph::NodeHandle> current_node_ = std::nullopt;
};

}  // namespace liger::rhi