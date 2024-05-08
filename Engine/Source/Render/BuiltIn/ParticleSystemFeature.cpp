/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ParticleSystemFeature.cpp
 * @date 2024-05-06
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

#include <Liger-Engine/Render/BuiltIn/ParticleSystemFeature.hpp>

namespace liger::render {

ParticleSystemFeature::ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager)
    : device_(device),
      emit_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleEmit.lshader")),
      update_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleUpdate.lshader")),
      render_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleRender.lshader")) {}

void ParticleSystemFeature::SetupRenderGraph(liger::rhi::RenderGraphBuilder& builder) {
  builder.BeginCompute("SimulateParticles");
  builder.EndCompute();
}

void ParticleSystemFeature::LinkRenderJobs(rhi::RenderGraph& graph) {
  graph.SetJob("SimulateParticles", [this](liger::rhi::ICommandBuffer& cmds) {
    if (emit_shader_.GetState() != asset::State::Loaded ||
        update_shader_.GetState() != asset::State::Loaded ||
        render_shader_.GetState() != asset::State::Loaded) {
      return;
    }

    emit_shader_->BindPipeline(cmds);
    for (auto& particle_system : particle_systems_) {
      emit_shader_->SetBuffer("ParticleSystem", particle_system.ubo_particle_system->GetUniformDescriptorBinding());
      emit_shader_->SetBuffer("Particles",      particle_system.sbo_particles->GetStorageDescriptorBinding());
      emit_shader_->SetBuffer("FreeList",       particle_system.sbo_free_list->GetStorageDescriptorBinding());
      emit_shader_->BindPushConstants(cmds);

      cmds.Dispatch((particle_system.particles + 63) / 64, 1, 1);
    }

    update_shader_->BindPipeline(cmds);
    for (auto& particle_system : particle_systems_) {
      update_shader_->SetBuffer("ParticleSystem", particle_system.ubo_particle_system->GetUniformDescriptorBinding());
      update_shader_->SetBuffer("Particles",      particle_system.sbo_particles->GetStorageDescriptorBinding());
      update_shader_->SetBuffer("FreeList",       particle_system.sbo_free_list->GetStorageDescriptorBinding());
      update_shader_->BindPushConstants(cmds);

      cmds.Dispatch((particle_system.particles + 63) / 64, 1, 1);
    }
  });
}

void ParticleSystemFeature::SetupLayerJobs(LayerMap& layer_map) {
  layer_map["Transparent"]->Emplace([this](rhi::ICommandBuffer& cmds) {
    if (emit_shader_.GetState() != asset::State::Loaded ||
        update_shader_.GetState() != asset::State::Loaded ||
        render_shader_.GetState() != asset::State::Loaded) {
      return;
    }

    render_shader_->BindPipeline(cmds);
    for (auto& particle_system : particle_systems_) {
      render_shader_->SetBuffer("ParticleSystem", particle_system.ubo_particle_system->GetUniformDescriptorBinding());
      render_shader_->SetBuffer("Particles",      particle_system.sbo_particles->GetStorageDescriptorBinding());
      render_shader_->BindPushConstants(cmds);

      cmds.DrawIndexed(4, 0, 0, particle_system.particles);
    }
  });
}

void ParticleSystemFeature::SetupEntitySystems(liger::ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void ParticleSystemFeature::Run(const ParticleSystemComponent& emitter, RuntimeParticleSystemData& runtime_data) {
  bool valid = runtime_data.ubo_particle_system && runtime_data.sbo_particles && runtime_data.sbo_free_list;
  if (!valid) {
    auto idx = particle_systems_.size();

    runtime_data.particles = emitter.max_particles;

    runtime_data.ubo_particle_system = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = sizeof(emitter),
      .usage       = rhi::DeviceResourceState::UniformBuffer,
      .cpu_visible = true,
      .name        = fmt::format("RuntimeParticleSystemData::ubo_particle_system[{0}]", idx)
    });

    runtime_data.sbo_particles = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = emitter.max_particles * sizeof(Particle),
      .usage       = rhi::DeviceResourceState::StorageBuffer,
      .cpu_visible = false,
      .name        = fmt::format("RuntimeParticleSystemData::sbo_particles[{0}]", idx)
    });

    runtime_data.sbo_free_list = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = sizeof(int32_t) + emitter.max_particles * sizeof(int32_t),
      .usage       = rhi::DeviceResourceState::StorageBuffer,
      .cpu_visible = false,
      .name        = fmt::format("RuntimeParticleSystemData::sbo_free_list[{0}]", idx)
    });
  }
}

}  // namespace liger::render