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

struct ParticleEmitterInfo {
  uint32_t                                      max_particles {128U};
  float                                         spawn_rate    {32.0f};
  float                                         lifetime      {2.0f};

  glm::vec3                                     acceleration  {0.0f};
  glm::vec3                                     velocity_min  {-0.4f, 0.3f, -0.4f};
  glm::vec3                                     velocity_max  { 0.4f, 1.5f,  0.4f};

  glm::vec4                                     color_start   {1.0f,  0.9f, 0.2f, 1.0f};
  glm::vec4                                     color_end     {1.0f,  0.7f, 0.6f, 0.4f};

  float                                         size_start    {0.025f};
  float                                         size_end      {0.01f};

  asset::Handle<std::unique_ptr<rhi::ITexture>> texture_atlas;
  uint32_t                                      atlas_size_x  {0U};
  uint32_t                                      atlas_size_y  {0U};
};

struct Particle {
  SHADER_STRUCT_MEMBER(glm::vec3) position;
  SHADER_STRUCT_MEMBER(glm::vec3) velocity;
  SHADER_STRUCT_MEMBER(glm::vec4) color;
  SHADER_STRUCT_MEMBER(float)     size;
  SHADER_STRUCT_MEMBER(float)     lifetime;
};

struct RuntimeParticleEmitterHandle {
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  uint32_t runtime_handle{kInvalid};
};

class ParticleSystemFeature : public IFeature {
 public:
  static constexpr uint32_t kMaxParticleSystems     = 64U;
  static constexpr uint32_t kMaxParticlesPerEmitter = 128U;

  explicit ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager, const FrameTimer& frame_timer);
  ~ParticleSystemFeature() override = default;

  std::string_view Name() const override { return "ParticleSystemFeature"; }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;
  void AddLayerJobs(LayerMap& layer_map) override;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;

  RuntimeParticleEmitterHandle Add(const ParticleEmitterInfo& emitter_info);
  void Update(RuntimeParticleEmitterHandle handle, const ParticleEmitterInfo& emitter_info, const glm::mat4& transform);

 private:
  struct RenderGraphVersions {
    rhi::RenderGraph::ResourceVersion emit_free_list;
    rhi::RenderGraph::ResourceVersion emit_particles;

    rhi::RenderGraph::ResourceVersion update_free_list;
    rhi::RenderGraph::ResourceVersion update_particles;
    rhi::RenderGraph::ResourceVersion update_draw_command;
    rhi::RenderGraph::ResourceVersion update_draw_particle_indices;

    rhi::RenderGraph::ResourceVersion render_particles;
    rhi::RenderGraph::ResourceVersion render_draw_command;
    rhi::RenderGraph::ResourceVersion render_draw_particle_indices;
  };

  struct ParticleEmitterUBO {
    SHADER_STRUCT_MEMBER(uint32_t)                      max_particles;
    SHADER_STRUCT_MEMBER(float)                         spawn_rate;
    SHADER_STRUCT_MEMBER(float)                         lifetime;

    SHADER_STRUCT_MEMBER(glm::vec3)                     acceleration;
    SHADER_STRUCT_MEMBER(glm::vec3)                     velocity_min;
    SHADER_STRUCT_MEMBER(glm::vec3)                     velocity_max;

    SHADER_STRUCT_MEMBER(glm::vec4)                     color_start;
    SHADER_STRUCT_MEMBER(glm::vec4)                     color_end;

    SHADER_STRUCT_MEMBER(float)                         size_start;
    SHADER_STRUCT_MEMBER(float)                         size_end;

    SHADER_STRUCT_MEMBER(rhi::TextureDescriptorBinding) binding_atlas{rhi::TextureDescriptorBinding::Invalid};
    SHADER_STRUCT_MEMBER(uint32_t)                      atlas_size_x;
    SHADER_STRUCT_MEMBER(uint32_t)                      atlas_size_y;
    SHADER_STRUCT_MEMBER(float)                         atlas_inv_size_x;
    SHADER_STRUCT_MEMBER(float)                         atlas_inv_size_y;

    ParticleEmitterUBO() = default;
    explicit ParticleEmitterUBO(const ParticleEmitterInfo& info)
        : max_particles(info.max_particles),
          spawn_rate(info.spawn_rate),
          lifetime(info.lifetime),
          acceleration(info.acceleration),
          velocity_min(info.velocity_min),
          velocity_max(info.velocity_max),
          color_start(info.color_start),
          color_end(info.color_end),
          size_start(info.size_start),
          size_end(info.size_end),
          atlas_size_x(info.atlas_size_x),
          atlas_size_y(info.atlas_size_y),
          atlas_inv_size_x(1.0f / static_cast<float>(info.atlas_size_x)),
          atlas_inv_size_y(1.0f / static_cast<float>(info.atlas_size_y)) {
      if (info.texture_atlas) {
        auto& atlas   = *info.texture_atlas->get();
        binding_atlas = atlas.GetSampledDescriptorBinding();
      }
    }
  };

  struct Instance {
    rhi::UniqueMappedBuffer<ParticleEmitterUBO>  ubo_emitter;
    std::unique_ptr<rhi::IBuffer>                sbo_particles;
    std::unique_ptr<rhi::IBuffer>                sbo_free_list;
    std::unique_ptr<rhi::IBuffer>                sbo_draw_command;
    std::unique_ptr<rhi::IBuffer>                sbo_draw_particle_indices;

    glm::mat4                                    transform;
    float                                        pending_spawn{0.0f};
    uint32_t                                     max_particles{0U};
    bool                                         initialized{false};
  };

  bool Initialized() const;

  rhi::IDevice&                 device_;
  const FrameTimer&             frame_timer_;

  asset::Handle<shader::Shader> emit_shader_;
  asset::Handle<shader::Shader> update_shader_;
  asset::Handle<shader::Shader> render_shader_;

  std::vector<Instance>         instances_;
  std::unique_ptr<rhi::IBuffer> sbo_init_free_list_;
  RenderGraphVersions           rg_versions_;
};

}  // namespace liger::render