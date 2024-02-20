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
#include <liger/render/rhi/resource_version_registry.hpp>
#include <liger/render/rhi/texture.hpp>

#include <optional>
#include <string_view>

namespace liger::rhi {

class IDevice;

class RenderGraph {
 public:
  struct TextureResource {
    ITexture* texture {nullptr};
    uint32_t  view    {kTextureDefaultViewIdx};
  };

  using BufferResource          = IBuffer*;
  using ResourceVersionRegistry = ResourceVersionRegistry<TextureResource, BufferResource>;
  using ResourceVersion         = ResourceVersionRegistry::ResourceVersion;

  class IJob {
   public:
    virtual ~IJob() = default;
    virtual void Execute(ICommandBuffer& cmds) = 0;
  };

  virtual ~RenderGraph() = default;

  TextureResource GetTexture(ResourceVersion version);
  BufferResource GetBuffer(ResourceVersion version);

  virtual void ReimportTexture(ResourceVersion version, TextureResource new_texture) = 0;
  virtual void ReimportBuffer(ResourceVersion version, BufferResource new_buffer) = 0;

  void SetJob(std::string_view node_name, std::unique_ptr<IJob> job);

  virtual void Execute(IDevice& device) = 0;

 protected:
  struct ResourceRead {
    ResourceVersion     version;
    DeviceResourceState state;
  };

  struct ResourceWrite {
    ResourceVersion     version;
    DeviceResourceState state;
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
    std::unique_ptr<IJob>      job;
  };

  using NodeGraph       = DAG<Node>;
  using NodeHandle      = NodeGraph::NodeHandle;
  using DependencyLevel = NodeGraph::Depth;

  virtual void Compile(IDevice& device) = 0;

  NodeHandle GetSortedNode(uint32_t sorted_idx) const;
  DependencyLevel GetDependencyLevel(NodeHandle node_handle) const;

  NodeGraph                                           dag_;
  NodeGraph::SortedList                               sorted_nodes_;
  NodeGraph::DepthList                                node_dependency_levels_;
  uint32_t                                            max_dependency_level_;
  ResourceVersionRegistry                             resource_version_registry_;
  std::unordered_map<ResourceVersion, ITexture::Info> transient_texture_infos_;
  std::unordered_map<ResourceVersion, IBuffer::Info>  transient_buffer_infos_;

  friend class RenderGraphBuilder;
};

class RenderGraphBuilder {
 public:
  using ResourceVersion = RenderGraph::ResourceVersion;

  explicit RenderGraphBuilder(std::unique_ptr<RenderGraph> graph);

  RenderGraphBuilder(const RenderGraphBuilder& other) = delete;
  RenderGraphBuilder& operator=(const RenderGraphBuilder& other) = delete;

  ResourceVersion DeclareTransientTexture(const ITexture::Info& info);
  ResourceVersion DeclareTransientBuffer(const IBuffer::Info& info);

  ResourceVersion ImportTexture(RenderGraph::TextureResource texture);
  ResourceVersion ImportBuffer(RenderGraph::BufferResource buffer);

  void BeginRenderPass(std::string_view           name,
                       ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kGraphics);
  void EndRenderPass();

  void BeginCompute(std::string_view name, bool async = false,
                    ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kCompute);
  void EndCompute();

  void BeginTransfer(std::string_view name, bool async = false,
                     ICommandBuffer::Capability capabilities = ICommandBuffer::Capability::kTransfer);
  void EndTransfer();

  ResourceVersion AddColorTarget(ResourceVersion texture);
  ResourceVersion SetDepthStencil(ResourceVersion texture);
  void SampleTexture(ResourceVersion texture);

  void ReadBuffer(ResourceVersion buffer, DeviceResourceState usage);

  [[nodiscard]] std::unique_ptr<RenderGraph> Build(IDevice& device);
 
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