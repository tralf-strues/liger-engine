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
#include <Liger-Engine/Core/Time.hpp>
#include <Liger-Engine/ECS/DefaultComponents.hpp>
#include <Liger-Engine/RHI/MappedBuffer.hpp>
#include <Liger-Engine/Render/BuiltIn/CameraData.hpp>
#include <Liger-Engine/Render/Feature.hpp>
#include <Liger-Engine/ShaderSystem/Shader.hpp>

namespace liger::render {

struct ParticleSystemComponent {
  SHADER_STRUCT_MEMBER(uint32_t)  max_particles {2048};
  SHADER_STRUCT_MEMBER(float)     spawn_rate    {16.0f};
  SHADER_STRUCT_MEMBER(float)     lifetime      {2.0f};

  SHADER_STRUCT_MEMBER(glm::vec3) velocity_min  {-0.4f, 0.01f, -0.4f};
  SHADER_STRUCT_MEMBER(glm::vec3) velocity_max  { 0.4f, 1.5f,   0.4f};

  SHADER_STRUCT_MEMBER(glm::vec4) color_start   {1.0f,  0.9f, 0.2f, 1.0f};
  SHADER_STRUCT_MEMBER(glm::vec4) color_end     {1.0f,  0.7f, 0.6f, 0.7f};

  SHADER_STRUCT_MEMBER(float)     size_start    {0.07f};
  SHADER_STRUCT_MEMBER(float)     size_end      {0.01f};
};

struct Particle {
  SHADER_STRUCT_MEMBER(glm::vec3) position;
  SHADER_STRUCT_MEMBER(glm::vec3) velocity;
  SHADER_STRUCT_MEMBER(glm::vec4) color;
  SHADER_STRUCT_MEMBER(float)     size;
  SHADER_STRUCT_MEMBER(float)     lifetime;
};

struct RuntimeParticleSystemData {
  static constexpr uint32_t kInvalidRuntimeHandle = std::numeric_limits<uint32_t>::max();

  bool                                             initialized{false};
  uint32_t                                         particles{0};
  float                                            spawn_rate{0.0f};
  float                                            to_spawn{0.0f};
  glm::mat4                                        transform;
  rhi::SharedMappedBuffer<ParticleSystemComponent> ubo_particle_system;
  std::shared_ptr<rhi::IBuffer>                    sbo_particles;
  std::shared_ptr<rhi::IBuffer>                    sbo_free_list;
  uint32_t                                         runtime_handle{kInvalidRuntimeHandle};
};

class ParticleSystemFeature
    : public IFeature,
      public ecs::ComponentSystem<const ecs::WorldTransform, const ParticleSystemComponent, RuntimeParticleSystemData> {
 public:
  static constexpr uint32_t kMaxParticleSystems = 1024U;

  explicit ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager, const FrameTimer& frame_timer);
  ~ParticleSystemFeature() override = default;

  std::string_view Name() const override { return "ParticleSystemFeature"; }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;
  void AddLayerJobs(LayerMap& layer_map) override;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;

  void Run(const ecs::WorldTransform& transform, const ParticleSystemComponent& emitter,
           RuntimeParticleSystemData& runtime_data) override;

 private:
  rhi::IDevice&                          device_;
  asset::Handle<shader::Shader>          emit_shader_;
  asset::Handle<shader::Shader>          update_shader_;
  asset::Handle<shader::Shader>          render_shader_;

  const FrameTimer&                      frame_timer_;

  rhi::UniqueMappedBuffer<int32_t>       sbo_init_free_list_;
  rhi::UniqueMappedBuffer<glm::mat4>     ubo_transform_;

  std::vector<RuntimeParticleSystemData> particle_systems_;
};

}  // namespace liger::render