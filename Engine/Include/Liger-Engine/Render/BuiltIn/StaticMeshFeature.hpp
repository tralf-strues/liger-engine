/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file StaticMeshFeature.hpp
 * @date 2024-05-05
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

#include <Liger-Engine/Asset/Manager.hpp>
#include <Liger-Engine/ECS/DefaultComponents.hpp>
#include <Liger-Engine/RHI/ShaderAlignment.hpp>
#include <Liger-Engine/Render/Feature.hpp>
#include <Liger-Engine/ShaderSystem/Shader.hpp>

namespace liger::render {

struct Vertex3D {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec2 tex_coords;
};

/* Assets */
struct Material {
  struct UBO {
    glm::vec4 color;
  };

  std::unique_ptr<rhi::IBuffer> ubo;
  glm::vec4                     color;
};

struct Submesh {
  struct UBO {
    rhi::BufferDescriptorBinding binding_vertex_buffer;
    rhi::BufferDescriptorBinding binding_index_buffer;
    uint32_t                     vertex_count;
    uint32_t                     index_count;
    glm::vec4                    bounding_sphere;
  };

  std::unique_ptr<rhi::IBuffer> ubo;

  std::unique_ptr<rhi::IBuffer> vertex_buffer;
  std::unique_ptr<rhi::IBuffer> index_buffer;
  uint32_t                      vertex_count;
  uint32_t                      index_count;
  glm::vec4                     bounding_sphere;

  asset::Handle<Material>       material;
};

struct StaticMesh {
  std::vector<Submesh> submeshes;
};

struct StaticMeshComponent {
  static constexpr uint32_t kInvalidRuntimeHandle = std::numeric_limits<uint32_t>::max();

  asset::Handle<StaticMesh> mesh;
  std::vector<uint32_t>     runtime_submesh_handles;
};

class StaticMeshFeature
    : public IFeature,
      public ecs::ComponentSystem<const ecs::WorldTransform, StaticMeshComponent> {
 public:
  explicit StaticMeshFeature(rhi::IDevice& device, asset::Manager& asset_manager);
  ~StaticMeshFeature() override = default;

  std::string_view Name() const override {
    return "StaticMeshFeature<const WorldTransform, StaticMeshComponent>";
  }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;
  void AddLayerJobs(LayerMap& layer_map) override;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;

  void Run(const ecs::WorldTransform& transform, StaticMeshComponent& static_mesh) override;

 private:
  static constexpr uint32_t kMaxObjects = 8192;
  static constexpr uint32_t kMaxMeshes  = 2048;

  struct Object {
    glm::mat4                    transform;
    rhi::BufferDescriptorBinding binding_mesh;
    rhi::BufferDescriptorBinding binding_material;
    uint32_t                     vertex_count;
    uint32_t                     index_count;
  };

  struct BatchedObject {
    uint32_t object_idx;
    uint32_t batch_idx;
  };

  struct CopyCmd {
    const rhi::IBuffer* src;
    uint64_t            size;
    uint64_t            dst_offset;
  };

  struct RenderGraphVersions {
    rhi::RenderGraph::ResourceVersion staging_buffer;

    rhi::RenderGraph::ResourceVersion objects;
    rhi::RenderGraph::ResourceVersion batched_objects;
    rhi::RenderGraph::ResourceVersion draw_commands;

    rhi::RenderGraph::ResourceVersion final_draw_commands;
    rhi::RenderGraph::ResourceVersion visible_object_indices;
  };

  uint32_t AddObject(Object object);
  void Rebuild(rhi::ICommandBuffer& cmds);

  rhi::IDevice&                 device_;
  std::vector<Object>           objects_;
  bool                          objects_added_{false};
  std::vector<uint32_t>         pending_remove_;
  std::unordered_set<uint32_t>  free_list_;

  std::vector<BatchedObject>    batched_objects_;
  std::vector<rhi::DrawCommand> draw_commands_;

  asset::Handle<shader::Shader> cull_shader_;
  asset::Handle<shader::Shader> render_shader_;
  std::unique_ptr<rhi::IBuffer> sbo_objects_;
  std::unique_ptr<rhi::IBuffer> sbo_batched_objects_;
  std::unique_ptr<rhi::IBuffer> sbo_draw_commands_;

  std::vector<rhi::IBuffer*>    index_buffers_per_object_;
  std::unique_ptr<rhi::IBuffer> merged_index_buffer_;
  uint64_t                      merged_index_buffer_total_size_;
  std::vector<CopyCmd>          index_buffer_copies_;

  RenderGraphVersions           rg_versions_;
};

}  // namespace liger::render