/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ClusteredLightFeature.cpp
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

#include <Liger-Engine/Render/BuiltIn/ClusteredLightFeature.hpp>

namespace liger::render {

ClusteredLightFeature::ClusteredLightFeature(asset::Manager& asset_manager)
    : gen_volumes_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ClusteredLightGenVolumes.lshader")),
      cull_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ClusteredLightCull.lshader")) {}

void ClusteredLightFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  rg_versions_.point_lights = builder.DeclareTransientBuffer(rhi::IBuffer::Info {
    .size        = sizeof(PointLight) * 64U,
    .usage       = rhi::DeviceResourceState::StorageBufferRead,
    .cpu_visible = true,
    .name        = "Point lights"
  });

  rg_versions_.pre_cluster_volumes = builder.DeclareTransientBuffer(rhi::IBuffer::Info {
    .size        = sizeof(AABB) * TotalClustersCount(),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = "Cluster Volumes"
  });

  rg_versions_.contributing_light_indices = builder.DeclareTransientBuffer(rhi::IBuffer::Info {
    .size        = sizeof(uint32_t) + sizeof(uint32_t) * TotalClustersCount() * kMaxLightsPerCluster,
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = "Contributing Light Indices"
  });

  rg_versions_.light_clusters = builder.DeclareTransientBuffer(rhi::IBuffer::Info {
    .size        = sizeof(LightCluster) * TotalClustersCount(),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = "Light Clusters"
  });

  builder.BeginCompute("Clustered Light Prepare");
  rg_versions_.post_cluster_volumes = builder.ReadWriteBuffer(rg_versions_.pre_cluster_volumes, rhi::DeviceResourceState::StorageBufferReadWrite);
  builder.SetJob([this](rhi::RenderGraph& graph, auto& context, rhi::ICommandBuffer& cmds) {
    if (gen_volumes_shader_.GetState() != asset::State::Loaded && !updated_) {
      return;
    }

    auto output_texture      = graph.GetTexture(context.template Get<OutputTexture>().rg_final_color);
    auto screen_resolution   = output_texture.texture->GetInfo().extent;

    auto sbo_point_lights    = graph.GetBuffer(rg_versions_.point_lights);
    auto sbo_cluster_volumes = graph.GetBuffer(rg_versions_.pre_cluster_volumes);

    gen_volumes_shader_->BindPipeline(cmds);

    gen_volumes_shader_->SetBuffer("CameraData",              context.template Get<CameraDataBinding>().binding_ubo);
    gen_volumes_shader_->SetBuffer("PointLights",             sbo_point_lights->GetStorageDescriptorBinding());
    gen_volumes_shader_->SetBuffer("ClusterVolumes",          sbo_cluster_volumes->GetStorageDescriptorBinding());
    gen_volumes_shader_->SetPushConstant("clusters_count",    clusters_count_);
    gen_volumes_shader_->SetPushConstant("screen_resolution", glm::uvec2(screen_resolution.x, screen_resolution.y));

    gen_volumes_shader_->BindPushConstants(cmds);
    cmds.Dispatch(clusters_count_.x, clusters_count_.y, clusters_count_.z);
  });
  builder.EndCompute();

  builder.BeginCompute("Clustered Light Cull");
  builder.ReadBuffer(rg_versions_.post_cluster_volumes,            rhi::DeviceResourceState::StorageBufferRead);
  builder.ReadBuffer(rg_versions_.point_lights, rhi::DeviceResourceState::StorageBufferRead);
  builder.ReadBuffer(rg_versions_.light_clusters, rhi::DeviceResourceState::StorageBufferWrite);
  auto final_contributing_light_indices = builder.ReadWriteBuffer(rg_versions_.contributing_light_indices,
                                                                  rhi::DeviceResourceState::StorageBufferReadWrite);
  builder.SetJob([this](rhi::RenderGraph& graph, auto& context, rhi::ICommandBuffer& cmds) {
    if (cull_shader_.GetState() != asset::State::Loaded) {
      return;
    }

    auto output_texture                 = graph.GetTexture(context.template Get<OutputTexture>().rg_final_color);
    auto screen_resolution              = output_texture.texture->GetInfo().extent;

    auto sbo_point_lights               = graph.GetBuffer(rg_versions_.point_lights);
    auto sbo_cluster_volumes            = graph.GetBuffer(rg_versions_.pre_cluster_volumes);
    auto sbo_light_clusters             = graph.GetBuffer(rg_versions_.light_clusters);
    auto sbo_contributing_light_indices = graph.GetBuffer(rg_versions_.contributing_light_indices);

    auto* raw_lights = sbo_point_lights->MapMemory();
    std::memcpy(raw_lights, point_lights_.data(), sizeof(PointLight) * point_lights_.size());
    sbo_point_lights->UnmapMemory();

    cull_shader_->BindPipeline(cmds);

    cull_shader_->SetBuffer("CameraData",               context.template Get<CameraDataBinding>().binding_ubo);
    cull_shader_->SetBuffer("PointLights",              sbo_point_lights->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("ClusterVolumes",           sbo_cluster_volumes->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("LightClusters",            sbo_light_clusters->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("ContributingLightIndices", sbo_contributing_light_indices->GetStorageDescriptorBinding());
    cull_shader_->SetPushConstant("light_count",        static_cast<uint32_t>(point_lights_.size()));
    cull_shader_->SetPushConstant("screen_resolution",  glm::uvec2(screen_resolution.x, screen_resolution.y));

    cull_shader_->BindPushConstants(cmds);
    cmds.Dispatch(clusters_count_.x, clusters_count_.y, clusters_count_.z);
  });
  builder.EndCompute();

  builder.GetContext().Insert(ClusteredLightData {
    .clusters_count = clusters_count_,

    .rg_point_lights = rg_versions_.point_lights,
    .rg_contributing_light_indices = final_contributing_light_indices,
    .rg_light_clusters = rg_versions_.light_clusters
  });
}

void ClusteredLightFeature::SetupEntitySystems(ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void ClusteredLightFeature::Run(const ecs::WorldTransform& transform, const PointLightInfo& point_light) {
  point_lights_.emplace_back(PointLight {
    .ws_position = transform.position,
    .radius      = point_light.radius,
    .color       = point_light.color,
    .intensity   = point_light.intensity,
  });
}

void ClusteredLightFeature::PreRender(rhi::IDevice&, rhi::RenderGraph& graph, rhi::Context& context) {
  auto output_extent = graph.GetTexture(context.template Get<OutputTexture>().rg_final_color).texture->GetInfo().extent;

  glm::uvec3 new_clusters_count{
    (output_extent.x + kClusterSizeXY - 1) / kClusterSizeXY,
    (output_extent.y + kClusterSizeXY - 1) / kClusterSizeXY,
    32U,
  };

  if (clusters_count_ != new_clusters_count) {
    clusters_count_ = new_clusters_count;
    updated_        = true;
  }

  graph.UpdateTransientBufferSize(rg_versions_.point_lights, point_lights_.size() * sizeof(PointLight));
  graph.UpdateTransientBufferSize(rg_versions_.pre_cluster_volumes, sizeof(AABB) * TotalClustersCount());
  graph.UpdateTransientBufferSize(rg_versions_.contributing_light_indices, sizeof(uint32_t) * (1U + TotalClustersCount()) * kMaxLightsPerCluster);
  graph.UpdateTransientBufferSize(rg_versions_.light_clusters, TotalClustersCount() * sizeof(LightCluster));

  const auto& camera = context.template Get<CameraData>();
  context.Insert(ClusteredLightData {
    .clusters_count = clusters_count_,

    .cluster_z_params {
      float(clusters_count_.z) / glm::log2(camera.far / camera.near),
      -glm::log2(camera.near) * (float(clusters_count_.z) / glm::log2(camera.far / camera.near))
    },

    .rg_point_lights = rg_versions_.point_lights,
    .rg_contributing_light_indices = rg_versions_.contributing_light_indices,
    .rg_light_clusters = rg_versions_.light_clusters
  });
}

void ClusteredLightFeature::PostRender(rhi::IDevice&, rhi::RenderGraph&, rhi::Context&) {
  point_lights_.clear();
  updated_ = false;
}

uint32_t ClusteredLightFeature::TotalClustersCount() const {
  return clusters_count_.x * clusters_count_.y * clusters_count_.z;
}

}  // namespace liger::render