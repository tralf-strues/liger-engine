/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ParticleSystemFeature.hpp
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

struct ParticleSystemComponent {
  SHADER_STRUCT_MEMBER(uint32_t)  max_particles {2048};
  SHADER_STRUCT_MEMBER(float)     spawn_rate    {8.0f};
  SHADER_STRUCT_MEMBER(float)     lifetime      {1.0f};

  SHADER_STRUCT_MEMBER(glm::vec3) velocity_min  {0.0f};
  SHADER_STRUCT_MEMBER(glm::vec3) velocity_max  {0.5f, 1.0f, 0.5f};

  SHADER_STRUCT_MEMBER(glm::vec4) color_start   {1.0f};
  SHADER_STRUCT_MEMBER(glm::vec4) color_end     {1.0f, 1.0f, 1.0f, 0.0f};

  SHADER_STRUCT_MEMBER(float)     size_start    {1.0f};
  SHADER_STRUCT_MEMBER(float)     size_end      {0.5f};
};

struct Particle {
  SHADER_STRUCT_MEMBER(glm::vec3) position;
  SHADER_STRUCT_MEMBER(glm::vec3) velocity;
  SHADER_STRUCT_MEMBER(glm::vec4) color;
  SHADER_STRUCT_MEMBER(float)     size;
  SHADER_STRUCT_MEMBER(float)     lifetime;
};

struct RuntimeParticleSystemData {
  uint32_t                      particles;
  std::shared_ptr<rhi::IBuffer> ubo_particle_system;
  std::shared_ptr<rhi::IBuffer> sbo_particles;
  std::shared_ptr<rhi::IBuffer> sbo_free_list;
};

class ParticleSystemFeature : public IFeature,
                              public ecs::ComponentSystem<const ParticleSystemComponent, RuntimeParticleSystemData> {
 public:
  explicit ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager);
  ~ParticleSystemFeature() override = default;

  std::string_view Name() const override { return "ParticleSystemFeature"; }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;
  void LinkRenderJobs(rhi::RenderGraph& graph) override;
  void SetupLayerJobs(LayerMap& layer_map) override;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;

  void Run(const ParticleSystemComponent& emitter, RuntimeParticleSystemData& runtime_data) override;

 private:
  rhi::IDevice&                          device_;
  asset::Handle<shader::Shader>          emit_shader_;
  asset::Handle<shader::Shader>          update_shader_;
  asset::Handle<shader::Shader>          render_shader_;
  std::vector<RuntimeParticleSystemData> particle_systems_;
};

}  // namespace liger::render