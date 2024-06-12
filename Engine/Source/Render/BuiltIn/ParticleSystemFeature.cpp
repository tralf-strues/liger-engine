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

class InitializeParticleEmitter : public ecs::ExclusiveComponentSystem<const ParticleEmitterInfo> {
 public:
  explicit InitializeParticleEmitter(ParticleSystemFeature& feature) : feature_(feature) {}
  ~InitializeParticleEmitter() override = default;

  void Setup(entt::registry& registry) override {
    registry.on_construct<ParticleEmitterInfo>().connect<&InitializeParticleEmitter::OnAttach>(this);
  }

  void OnAttach(entt::registry& registry, entt::entity entity) {
    const auto& emitter_info = registry.get<ParticleEmitterInfo>(entity);

    auto* runtime_handle_ptr = registry.try_get<RuntimeParticleEmitterHandle>(entity);
    if (runtime_handle_ptr == nullptr) {
      registry.emplace<RuntimeParticleEmitterHandle>(entity, feature_.Add(emitter_info));
    }
  }

  void Run(entt::registry& registry, entt::entity entity, const ParticleEmitterInfo& emitter_info) override {}

  std::string_view Name() const override { return "InitializeParticleEmitter<const ParticleEmitterInfo>"; }

 private:
  ParticleSystemFeature& feature_;
};

class UpdateParticleEmitter : public ecs::ComponentSystem<const RuntimeParticleEmitterHandle,
                                                          const ParticleEmitterInfo,
                                                          const ecs::WorldTransform> {
 public:
  explicit UpdateParticleEmitter(ParticleSystemFeature& feature) : feature_(feature) {}
  ~UpdateParticleEmitter() override = default;

  void Run(const RuntimeParticleEmitterHandle& handle,
           const ParticleEmitterInfo& emitter_info,
           const ecs::WorldTransform& transform) override {
    feature_.Update(handle, emitter_info, transform.Matrix());
  }

  std::string_view Name() const override {
    return "UpdateParticleEmitter<const RuntimeParticleEmitterHandle, const ParticleEmitterInfo, const WorldTransform>";
  }

 private:
  ParticleSystemFeature& feature_;
};

ParticleSystemFeature::ParticleSystemFeature(rhi::IDevice& device, asset::Manager& asset_manager,
                                             const FrameTimer& frame_timer)
    : device_(device),
      emit_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleEmit.lshader")),
      update_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleUpdate.lshader")),
      render_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.ParticleRender.lshader")),
      frame_timer_(frame_timer) {
  instances_.reserve(kMaxParticleSystems);

  sbo_init_free_list_ = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = (kMaxParticlesPerEmitter + 1U) * sizeof(int32_t),
    .usage       = rhi::DeviceResourceState::TransferSrc,
    .cpu_visible = true,
    .name        = "ParticleSystemFeature::sbo_init_free_list_"
  });

  auto* mapped_free_list = reinterpret_cast<int32_t*>(sbo_init_free_list_->MapMemory());
  mapped_free_list[0U] = kMaxParticlesPerEmitter; // NOTE (tralf-strues): first element is the count of free particles
  for (uint32_t i = 0; i < kMaxParticlesPerEmitter; ++i) {
    mapped_free_list[i + 1U] = i; 
  }
  sbo_init_free_list_->UnmapMemory();
}

void ParticleSystemFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  rg_versions_.emit_free_list = builder.DeclareImportBufferPack("Particle Free List",
                                                                rhi::DeviceResourceState::Undefined,
                                                                rhi::DeviceResourceState::Undefined);
  rg_versions_.emit_particles = builder.DeclareImportBufferPack("Particles",
                                                                rhi::DeviceResourceState::Undefined,
                                                                rhi::DeviceResourceState::Undefined);
  rg_versions_.update_draw_command = builder.DeclareImportBufferPack("Particle Draw Cmd",
                                                                rhi::DeviceResourceState::Undefined,
                                                                rhi::DeviceResourceState::Undefined);
  rg_versions_.update_draw_particle_indices = builder.DeclareImportBufferPack("Draw Particle Indices",
                                                                rhi::DeviceResourceState::Undefined,
                                                                rhi::DeviceResourceState::Undefined);

  /* Emit */
  builder.BeginCompute("Particle Emit");

  rg_versions_.update_free_list = builder.ReadWriteBuffer(rg_versions_.emit_free_list, rhi::DeviceResourceState::StorageBufferReadWrite);
  rg_versions_.update_particles = builder.ReadWriteBuffer(rg_versions_.emit_particles, rhi::DeviceResourceState::StorageBufferWrite);

  builder.SetJob([this](rhi::RenderGraph& graph, auto&, auto& cmds) {
    if (!Initialized()) {
      return;
    }

    for (auto& instance : instances_) {
      if (instance.initialized) {
        continue;
      }

      graph.GetBufferPack(rg_versions_.emit_free_list).buffers->push_back(instance.sbo_free_list.get());
      graph.GetBufferPack(rg_versions_.emit_particles).buffers->push_back(instance.sbo_particles.get());
      graph.GetBufferPack(rg_versions_.update_draw_command).buffers->push_back(instance.sbo_draw_command.get());
      graph.GetBufferPack(rg_versions_.update_draw_particle_indices).buffers->push_back(instance.sbo_draw_particle_indices.get());

      cmds.CopyBuffer(sbo_init_free_list_.get(), instance.sbo_free_list.get(), instance.sbo_free_list->GetInfo().size);

      instance.initialized = true;
    }

    emit_shader_->BindPipeline(cmds);
    for (auto& instance : instances_) {
      float particles_to_spawn = 0.0f;
      instance.pending_spawn = std::modf(instance.pending_spawn, &particles_to_spawn);

      emit_shader_->SetPushConstant("time",  frame_timer_.GetTimer().ElapsedMs());
      emit_shader_->SetBuffer("EmitterData", instance.ubo_emitter->GetUniformDescriptorBinding());
      emit_shader_->SetBuffer("Particles",   instance.sbo_particles->GetStorageDescriptorBinding());
      emit_shader_->SetBuffer("FreeList",    instance.sbo_free_list->GetStorageDescriptorBinding());
      emit_shader_->SetPushConstant("particles_to_spawn", uint32_t(particles_to_spawn));
      emit_shader_->BindPushConstants(cmds);

      cmds.Dispatch((uint32_t(particles_to_spawn) + 31) / 32, 1, 1);
    }
  });

  builder.EndCompute();

  /* Update */
  builder.BeginCompute("Particle Update");

  builder.ReadWriteBuffer(rg_versions_.update_free_list, rhi::DeviceResourceState::StorageBufferReadWrite);
  rg_versions_.render_particles             = builder.ReadWriteBuffer(rg_versions_.update_particles, rhi::DeviceResourceState::StorageBufferWrite);
  rg_versions_.render_draw_command          = builder.ReadWriteBuffer(rg_versions_.update_draw_command, rhi::DeviceResourceState::StorageBufferReadWrite);
  rg_versions_.render_draw_particle_indices = builder.ReadWriteBuffer(rg_versions_.update_draw_particle_indices, rhi::DeviceResourceState::StorageBufferWrite);

  builder.SetJob([this](auto&, auto&, auto& cmds) {
    if (!Initialized()) {
      return;
    }

    update_shader_->BindPipeline(cmds);
    update_shader_->SetPushConstant("delta_time", frame_timer_.DeltaTime());
    for (auto& instance : instances_) {
      update_shader_->SetBuffer("EmitterData",         instance.ubo_emitter->GetUniformDescriptorBinding());
      update_shader_->SetBuffer("Particles",           instance.sbo_particles->GetStorageDescriptorBinding());
      update_shader_->SetBuffer("FreeList",            instance.sbo_free_list->GetStorageDescriptorBinding());
      update_shader_->SetBuffer("Draw",                instance.sbo_draw_command->GetStorageDescriptorBinding());
      update_shader_->SetBuffer("DrawParticleIndices", instance.sbo_draw_particle_indices->GetStorageDescriptorBinding());
      update_shader_->BindPushConstants(cmds);

      cmds.Dispatch((instance.ubo_emitter.GetData()->max_particles + 63) / 64, 1, 1);
    }
  });

  builder.EndCompute();
}

void ParticleSystemFeature::AddLayerJobs(LayerMap& layer_map) {
  auto& layer = layer_map["Transparent"];

  layer->Emplace([this](rhi::RenderGraphBuilder& builder) {
    builder.ReadBuffer(rg_versions_.render_particles, rhi::DeviceResourceState::StorageBufferRead);
    builder.ReadBuffer(rg_versions_.render_draw_command, rhi::DeviceResourceState::IndirectArgument);
    builder.ReadBuffer(rg_versions_.render_draw_particle_indices, rhi::DeviceResourceState::StorageBufferRead);
  });

  layer->Emplace([this](auto&, rhi::Context& context, rhi::ICommandBuffer& cmds) {
    if (!Initialized()) {
      return;
    }

    render_shader_->BindPipeline(cmds);
    render_shader_->SetBuffer("CameraData", context.template Get<CameraDataBinding>().binding_ubo);
    for (auto& instance : instances_) {
      render_shader_->SetPushConstant("transform",     instance.transform);
      render_shader_->SetBuffer("EmitterData",         instance.ubo_emitter->GetUniformDescriptorBinding());
      render_shader_->SetBuffer("Particles",           instance.sbo_particles->GetStorageDescriptorBinding());
      render_shader_->SetBuffer("DrawParticleIndices", instance.sbo_draw_particle_indices->GetStorageDescriptorBinding());
      render_shader_->BindPushConstants(cmds);

      cmds.DrawIndirect(instance.sbo_draw_command.get(), 0U, sizeof(rhi::DrawCommand), 1U);
    }
  });
}

void ParticleSystemFeature::SetupEntitySystems(liger::ecs::SystemGraph& systems) {
  systems.Emplace(std::make_unique<InitializeParticleEmitter>(*this));
  systems.Emplace(std::make_unique<UpdateParticleEmitter>(*this));
}

RuntimeParticleEmitterHandle ParticleSystemFeature::Add(const ParticleEmitterInfo& emitter_info) {
  const auto idx = static_cast<uint32_t>(instances_.size());

  auto& instance = instances_.emplace_back();

  instance.max_particles = emitter_info.max_particles;

  instance.ubo_emitter = rhi::UniqueMappedBuffer<ParticleEmitterUBO>(
      device_,
      rhi::DeviceResourceState::UniformBuffer,
      fmt::format("ParticleSystemFeature::instances_[{0}]::ubo_emitter", idx)
  );

  instance.sbo_particles = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = instance.max_particles * sizeof(Particle),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = fmt::format("ParticleSystemFeature::instances_[{0}]::sbo_particles", idx)
  });

  instance.sbo_free_list = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = (instance.max_particles + 1U) * sizeof(int32_t),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = fmt::format("ParticleSystemFeature::instances_[{0}]::sbo_free_list", idx)
  });

  instance.sbo_draw_command = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = sizeof(rhi::DrawCommand),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite | rhi::DeviceResourceState::IndirectArgument,
    .cpu_visible = false,
    .name        = fmt::format("ParticleSystemFeature::instances_[{0}]::sbo_draw_command", idx)
  });

  instance.sbo_draw_particle_indices = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = instance.max_particles * sizeof(uint32_t),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = fmt::format("ParticleSystemFeature::instances_[{0}]::sbo_draw_particle_indices", idx)
  });

  return RuntimeParticleEmitterHandle{.runtime_handle = idx};
}

void ParticleSystemFeature::Update(RuntimeParticleEmitterHandle handle,
                                   const ParticleEmitterInfo& emitter_info,
                                   const glm::mat4& transform) {
  uint32_t idx = handle.runtime_handle;
  
  *instances_[idx].ubo_emitter.GetData() = ParticleEmitterUBO(emitter_info);
  instances_[idx].transform              = transform;
  instances_[idx].pending_spawn         += emitter_info.spawn_rate * frame_timer_.DeltaTime();
}

bool ParticleSystemFeature::Initialized() const {
  return emit_shader_.GetState()   == asset::State::Loaded &&
         update_shader_.GetState() == asset::State::Loaded &&
         render_shader_.GetState() == asset::State::Loaded;
}

}  // namespace liger::render