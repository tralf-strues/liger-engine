/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file StaticMeshFeature.cpp
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

#include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

#include <Liger-Engine/Render/LogChannel.hpp>

namespace liger::render {

StaticMeshFeature::StaticMeshFeature(rhi::IDevice& device, asset::Manager& asset_manager)
    : cull_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.StaticMeshCull.lshader")),
      render_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.StaticMeshRender.lshader")) {
  pending_remove_.reserve(kMaxObjects);
  free_list_.reserve(kMaxObjects);
  objects_.reserve(kMaxObjects);
  batched_objects_.reserve(kMaxObjects);
  draw_commands_.reserve(kMaxMeshes);

  sbo_objects_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxObjects * sizeof(Object),
    .usage       = rhi::DeviceResourceState::StorageBuffer | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_objects_"
  });

  sbo_batched_objects_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxObjects * sizeof(BatchedObject),
    .usage       = rhi::DeviceResourceState::StorageBuffer | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_batched_objects_"
  });

  sbo_draw_commands_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxMeshes * sizeof(rhi::DrawCommand),
    .usage       = rhi::DeviceResourceState::StorageBuffer | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_batched_objects_"
  });

  for (uint32_t object_idx = 0U; object_idx <= kMaxObjects; ++object_idx) {
    free_list_.insert(object_idx);
  }
}

void StaticMeshFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  using namespace rhi;

  rg_versions_.objects         = builder.ImportBuffer(sbo_objects_.get(),         DeviceResourceState::TransferDst, DeviceResourceState::StorageBuffer);
  rg_versions_.batched_objects = builder.ImportBuffer(sbo_batched_objects_.get(), DeviceResourceState::TransferDst, DeviceResourceState::StorageBuffer);
  rg_versions_.draw_commands   = builder.ImportBuffer(sbo_draw_commands_.get(),   DeviceResourceState::TransferDst, DeviceResourceState::IndirectArgument);

  rg_versions_.visible_object_indices = builder.DeclareTransientBuffer(IBuffer::Info {
    .size        = kMaxObjects * sizeof(uint32_t),
    .usage       = DeviceResourceState::StorageBuffer,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_visible_object_indices_"
  });

  rg_versions_.staging_buffer = builder.DeclareTransientBuffer(IBuffer::Info {
    .size        = sbo_objects_->GetInfo().size + sbo_batched_objects_->GetInfo().size + sbo_draw_commands_->GetInfo().size,
    .usage       = DeviceResourceState::TransferSrc,
    .cpu_visible = true,
    .name        = "StaticMeshFeature - staging_buffer"
  });

  builder.BeginTransfer("StaticMeshFeature - prepare buffers");
  builder.ReadBuffer(rg_versions_.staging_buffer,          DeviceResourceState::TransferSrc);
  builder.WriteBuffer(rg_versions_.objects,                DeviceResourceState::TransferDst);
  builder.WriteBuffer(rg_versions_.batched_objects,        DeviceResourceState::TransferDst);
  builder.WriteBuffer(rg_versions_.draw_commands,          DeviceResourceState::TransferDst);
  builder.SetJob([this](auto& graph, auto& context, auto& cmds) {
    if (!objects_added_ && pending_remove_.empty()) {
      return;
    }

    Rebuild();

    auto staging_buffer = graph.GetBuffer(rg_versions_.staging_buffer);
    auto* staging_data = staging_buffer->MapMemory();

    uint64_t objects_data_size         = objects_.size() * sizeof(objects_[0U]);
    uint64_t batched_objects_data_size = batched_objects_.size() * sizeof(batched_objects_[0U]);
    uint64_t draw_commands_data_size   = draw_commands_.size() * sizeof(draw_commands_[0U]);

    uint64_t offset = 0U;
    std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, objects_.data(), objects_data_size);
    cmds.CopyBuffer(staging_buffer, sbo_objects_.get(), objects_data_size, offset);
    offset += objects_data_size;
    std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, batched_objects_.data(), batched_objects_data_size);
    cmds.CopyBuffer(staging_buffer, sbo_batched_objects_.get(), batched_objects_data_size, offset);
    offset += batched_objects_data_size;
    std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, draw_commands_.data(), draw_commands_data_size);
    cmds.CopyBuffer(staging_buffer, sbo_draw_commands_.get(), draw_commands_data_size, offset);

    staging_buffer->UnmapMemory();
  });
  builder.EndTransfer();

  builder.BeginCompute("StaticMeshFeature - frustum cull");
  builder.ReadBuffer(rg_versions_.objects,                 DeviceResourceState::StorageBuffer);
  builder.ReadBuffer(rg_versions_.batched_objects,         DeviceResourceState::StorageBuffer);
  builder.WriteBuffer(rg_versions_.visible_object_indices, DeviceResourceState::StorageBuffer);
  builder.ReadBuffer(rg_versions_.draw_commands,           DeviceResourceState::IndirectArgument);
  builder.SetJob([this](auto& graph, auto& context, auto& cmds) {
    cull_shader_->BindPipeline(cmds);
    // TODO (tralf-strues): bind data!
    cmds.Dispatch((batched_objects_.size() + 63U) / 64U, 1U, 1U);
  });
  builder.EndCompute();
}

void StaticMeshFeature::SetupLayers(LayerMap& layer_map) {
  layer_map["Opaque"]->Emplace([this](auto& graph, auto& context, auto& cmds) {
    render_shader_->BindPipeline(cmds);
    // TODO (tralf-strues): bind data!
    cmds.DrawIndexedIndirect(sbo_draw_commands_.get(), 0U, sizeof(draw_commands_[0]), draw_commands_.size());
  });
}

void StaticMeshFeature::SetupEntitySystems(ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void StaticMeshFeature::Run(const ecs::WorldTransform& transform, StaticMeshComponent& static_mesh) {
  const uint32_t submeshes_count = static_mesh.mesh->submeshes.size();

  if (static_mesh.runtime_submesh_handles.size() != submeshes_count) {
    static_mesh.runtime_submesh_handles.resize(submeshes_count);

    for (uint32_t submesh_idx = 0U; submesh_idx < submeshes_count; ++submesh_idx) {
      const auto& submesh    = static_mesh.mesh->submeshes[submesh_idx];
      auto&       object_idx = static_mesh.runtime_submesh_handles[submesh_idx];

      object_idx = AddObject(Object {
        .binding_mesh     = submesh.ubo->GetUniformDescriptorBinding(),
        .binding_material = submesh.material->ubo->GetUniformDescriptorBinding(),
        .vertex_count     = submesh.vertex_count,
        .index_count      = submesh.index_count
      });
    }
  }

  for (auto object_idx : static_mesh.runtime_submesh_handles) {
    objects_[object_idx].transform = transform.Matrix();
  }
}

uint32_t StaticMeshFeature::AddObject(Object object) {
  LIGER_ASSERT(!free_list_.empty(), kLogChannelRender, "No more space left in StaticMeshFeature");

  auto it         = free_list_.begin();
  auto object_idx = *it;

  objects_[object_idx] = std::move(object);
  objects_added_       = true;

  free_list_.erase(it);
  return object_idx;
}

void StaticMeshFeature::Rebuild() {
  /* Add and remove objects */
  for (auto object_idx : pending_remove_) {
    free_list_.insert(object_idx);
  }
  pending_remove_.clear();

  /* Initialize batched objects list */
  batched_objects_.clear();

  for (uint32_t object_idx = 0U; object_idx < objects_.size(); ++object_idx) {
    if (free_list_.find(object_idx) != free_list_.end()) {
      continue;
    }

    batched_objects_.emplace_back(BatchedObject{.object_idx = object_idx});
  }

  std::sort(batched_objects_.begin(), batched_objects_.end(), [this](const BatchedObject& lhs, const BatchedObject& rhs) {
    return objects_[lhs.object_idx].binding_mesh < objects_[rhs.object_idx].binding_mesh;
  });

  /* Slice objects into batches (aka group objects with the same mesh data into a single instanced draw call) */
  draw_commands_.clear();

  for (uint32_t i = 0U, batch_idx = 0U; i < batched_objects_.size(); ++i) {
    if (i > 0 && objects_[batched_objects_[i - 1].object_idx].binding_mesh != objects_[batched_objects_[i].object_idx].binding_mesh) {
      draw_commands_.emplace_back(rhi::DrawCommand {
        .index_count    = objects_[batched_objects_[i].object_idx].index_count,
        .instance_count = 0U,  // NOTE (tralf-strues): this value is computed in culling shader
        .first_index    = 0U,
        .vertex_offset  = 0U,
        .first_instance = i,
      });

      ++batch_idx;
    }

    batched_objects_[i].batch_idx = batch_idx;
  }

  objects_added_ = false;
}

}  // namespace liger::render