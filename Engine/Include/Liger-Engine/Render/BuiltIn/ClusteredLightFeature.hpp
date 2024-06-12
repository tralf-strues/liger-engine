/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ClusteredLightFeature.hpp
 * @date 2024-06-03
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
#include <Liger-Engine/Render/BuiltIn/CameraData.hpp>
#include <Liger-Engine/Render/BuiltIn/ClusteredLightData.hpp>
#include <Liger-Engine/Render/BuiltIn/OutputTexture.hpp>
#include <Liger-Engine/Render/Feature.hpp>
#include <Liger-Engine/ShaderSystem/Shader.hpp>

namespace liger::render {

class ClusteredLightFeature : public IFeature, ecs::ComponentSystem<const ecs::WorldTransform, const PointLightInfo> {
 public:
  static constexpr uint32_t kClusterSizeXY       = 16U;
  static constexpr uint32_t kMaxLightsPerCluster = 512U;

  explicit ClusteredLightFeature(asset::Manager& asset_manager);
  ~ClusteredLightFeature() override = default;

  std::string_view Name() const override { return "ClusteredLightFeature"; }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;
  void Run(const ecs::WorldTransform& transform, const PointLightInfo& point_light) override;

  void PreRender(rhi::IDevice&, rhi::RenderGraph& graph, rhi::Context& context) override;
  void PostRender(rhi::IDevice&, rhi::RenderGraph&, rhi::Context&) override;

 private:
  struct PointLight {
    SHADER_STRUCT_MEMBER(glm::vec3) ws_position;
    SHADER_STRUCT_MEMBER(float)     radius;
    SHADER_STRUCT_MEMBER(glm::vec3) color;
    SHADER_STRUCT_MEMBER(float)     intensity;
  };

  struct AABB {
    SHADER_STRUCT_MEMBER(glm::vec4) min_point;
    SHADER_STRUCT_MEMBER(glm::vec4) max_point;
  };

  struct LightCluster {
    SHADER_STRUCT_MEMBER(uint32_t) offset;
    SHADER_STRUCT_MEMBER(uint32_t) count;
    SHADER_STRUCT_MEMBER(uint32_t) pad0;
    SHADER_STRUCT_MEMBER(uint32_t) pad1;
  };

  uint32_t TotalClustersCount() const;

  asset::Handle<shader::Shader> gen_volumes_shader_;
  asset::Handle<shader::Shader> cull_shader_;

  struct RenderGraphVersions {
    rhi::RenderGraph::ResourceVersion pre_cluster_volumes;
    rhi::RenderGraph::ResourceVersion post_cluster_volumes;

    rhi::RenderGraph::ResourceVersion point_lights;
    rhi::RenderGraph::ResourceVersion contributing_light_indices;
    rhi::RenderGraph::ResourceVersion light_clusters;
  };

  RenderGraphVersions     rg_versions_;
  glm::uvec3              clusters_count_{1U, 1U, 32U};
  bool                    updated_{false};
  std::vector<PointLight> point_lights_;
};

}  // namespace liger::render