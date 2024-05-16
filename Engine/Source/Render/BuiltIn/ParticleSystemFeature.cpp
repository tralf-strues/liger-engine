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

ParticleSystemFeature::ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager,
                                             const FrameTimer& frame_timer)
    : device_(device),
      emit_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleEmit.lshader")),
      update_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleUpdate.lshader")),
      render_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleRender.lshader")),
      frame_timer_(frame_timer) {
  ubo_transform_ = rhi::UniqueMappedBuffer<glm::mat4>(device_, rhi::DeviceResourceState::StorageBufferRead,
                                                      "ParticleSystemFeature::ubo_transform_", kMaxParticleSystems);
  sbo_init_free_list_ = rhi::UniqueMappedBuffer<int32_t>(device_, rhi::DeviceResourceState::TransferSrc,
                                                         "ParticleSystemFeature::sbo_init_free_list_", 2049U);

  sbo_init_free_list_.GetData()[0U] = 2048U;
  for (uint32_t i = 0; i < 2048U; ++i) {
    sbo_init_free_list_.GetData()[i + 1U] = i;
  }
}

void ParticleSystemFeature::SetupRenderGraph(liger::rhi::RenderGraphBuilder& builder) {
  builder.BeginCompute("SimulateParticles", false);
  builder.SetJob([this](auto&, auto&, auto& cmds) {
    if (emit_shader_.GetState() != asset::State::Loaded ||
        update_shader_.GetState() != asset::State::Loaded ||
        render_shader_.GetState() != asset::State::Loaded) {
      return;
    }

    for (auto& particle_system : particle_systems_) {
      if (!particle_system.initialized) {
        cmds.CopyBuffer(sbo_init_free_list_.get(), particle_system.sbo_free_list.get(), sbo_init_free_list_->GetInfo().size);
        //cmds.BufferBarrier(particle_system.sbo_free_list.get(), rhi::DeviceResourceState::TransferDst, rhi::DeviceResourceState::StorageBufferReadWrite);

        particle_system.initialized = true;
      }
    }

    emit_shader_->BindPipeline(cmds);
    emit_shader_->SetPushConstant("time", frame_timer_.AbsoluteTime());
    for (auto& particle_system : particle_systems_) {
      float particles_to_spawn = 0.0f;
      particle_system.to_spawn = std::modf(particle_system.to_spawn, &particles_to_spawn);

      emit_shader_->SetBuffer("EmitterData", particle_system.ubo_particle_system->GetUniformDescriptorBinding());
      emit_shader_->SetBuffer("Particles",   particle_system.sbo_particles->GetStorageDescriptorBinding());
      emit_shader_->SetBuffer("FreeList",    particle_system.sbo_free_list->GetStorageDescriptorBinding());
      emit_shader_->SetPushConstant("particles_to_spawn", uint32_t(particles_to_spawn));
      emit_shader_->BindPushConstants(cmds);

      cmds.Dispatch((particle_system.particles + 63) / 64, 1, 1);
      //cmds.BufferBarrier(particle_system.sbo_particles.get(), rhi::DeviceResourceState::StorageBuffer, rhi::DeviceResourceState::StorageBuffer);
      //cmds.BufferBarrier(particle_system.sbo_free_list.get(), rhi::DeviceResourceState::StorageBuffer, rhi::DeviceResourceState::StorageBuffer);
    }

    update_shader_->BindPipeline(cmds);
    update_shader_->SetPushConstant("delta_time", frame_timer_.DeltaTime());
    for (auto& particle_system : particle_systems_) {
      update_shader_->SetBuffer("EmitterData", particle_system.ubo_particle_system->GetUniformDescriptorBinding());
      update_shader_->SetBuffer("Particles",    particle_system.sbo_particles->GetStorageDescriptorBinding());
      update_shader_->SetBuffer("FreeList",    particle_system.sbo_free_list->GetStorageDescriptorBinding());
      update_shader_->BindPushConstants(cmds);

      cmds.Dispatch((particle_system.particles + 63) / 64, 1, 1);
      //cmds.BufferBarrier(particle_system.sbo_particles.get(), rhi::DeviceResourceState::StorageBuffer, rhi::DeviceResourceState::StorageBuffer);
      //cmds.BufferBarrier(particle_system.sbo_free_list.get(), rhi::DeviceResourceState::StorageBuffer, rhi::DeviceResourceState::StorageBuffer);
    }
  });
  builder.EndCompute();
}

void ParticleSystemFeature::AddLayerJobs(LayerMap& layer_map) {
  auto& layer = layer_map["Transparent"];

  // TODO: setup tasks

  layer->Emplace([this](auto&, rhi::Context& context, rhi::ICommandBuffer& cmds) {
    if (emit_shader_.GetState() != asset::State::Loaded ||
        update_shader_.GetState() != asset::State::Loaded ||
        render_shader_.GetState() != asset::State::Loaded) {
      return;
    }

    render_shader_->BindPipeline(cmds);
    render_shader_->SetBuffer("TransformData", ubo_transform_->GetStorageDescriptorBinding());
    render_shader_->SetBuffer("CameraData", context.Get<CameraDataBinding>().binding_ubo);
    for (size_t idx = 0U; idx < particle_systems_.size(); ++idx) {
      auto& particle_system = particle_systems_[idx];

      ubo_transform_.GetData()[idx] = particle_system.transform;

      render_shader_->SetPushConstant("transform_idx", static_cast<uint32_t>(idx));
      render_shader_->SetBuffer("Particles", particle_system.sbo_particles->GetStorageDescriptorBinding());
      render_shader_->BindPushConstants(cmds);

      cmds.Draw(4, 0, particle_system.particles, 0);
    }
  });
}

void ParticleSystemFeature::SetupEntitySystems(liger::ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void ParticleSystemFeature::Run(const ecs::WorldTransform& transform, const ParticleSystemComponent& emitter,
                                RuntimeParticleSystemData& runtime_data) {
  if (runtime_data.runtime_handle == RuntimeParticleSystemData::kInvalidRuntimeHandle) {
    auto idx = particle_systems_.size();

    runtime_data.particles = emitter.max_particles;

    runtime_data.ubo_particle_system = rhi::SharedMappedBuffer<ParticleSystemComponent>(
        device_, rhi::DeviceResourceState::UniformBuffer,
        fmt::format("RuntimeParticleSystemData::ubo_particle_system[{0}]", idx)
    );

    runtime_data.sbo_particles = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = emitter.max_particles * sizeof(Particle),
      .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
      .cpu_visible = false,
      .name        = fmt::format("RuntimeParticleSystemData::sbo_particles[{0}]", idx)
    });

    runtime_data.sbo_free_list = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = sizeof(int32_t) + emitter.max_particles * sizeof(int32_t),
      .usage       = rhi::DeviceResourceState::StorageBufferReadWrite | rhi::DeviceResourceState::TransferDst,
      .cpu_visible = false,
      .name        = fmt::format("RuntimeParticleSystemData::sbo_free_list[{0}]", idx)
    });

    runtime_data.runtime_handle = particle_systems_.size();
    particle_systems_.emplace_back(runtime_data);
  }

  uint32_t idx = runtime_data.runtime_handle;
  
  *particle_systems_[idx].ubo_particle_system.GetData() = emitter;
  particle_systems_[idx].transform  = transform.Matrix();
  particle_systems_[idx].spawn_rate = emitter.spawn_rate;
  particle_systems_[idx].to_spawn  += particle_systems_[idx].spawn_rate * frame_timer_.DeltaTime();
}

}  // namespace liger::render