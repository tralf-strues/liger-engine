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

#include <Liger-Engine/Render/BuiltIn/CameraData.hpp>
#include <Liger-Engine/Render/LogChannel.hpp>

namespace liger::render {

inline glm::vec4 NormalizePlane(const glm::vec4& plane) {
  return plane / glm::length(glm::vec3(plane));
}

StaticMeshFeature::StaticMeshFeature(rhi::IDevice& device, asset::Manager& asset_manager)
    : device_(device),
      cull_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.StaticMeshCull.lshader")),
      render_shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.StaticMeshRender.lshader")) {
  pending_remove_.reserve(kMaxObjects);
  free_list_.reserve(kMaxObjects);
  objects_.resize(kMaxObjects);
  index_buffers_per_object_.resize(kMaxObjects, nullptr);
  batched_objects_.reserve(kMaxObjects);
  draw_commands_.reserve(kMaxMeshes);
  index_buffer_copies_.reserve(kMaxMeshes);

  sbo_objects_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxObjects * sizeof(Object),
    .usage       = rhi::DeviceResourceState::StorageBufferRead | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_objects_"
  });

  sbo_batched_objects_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxObjects * sizeof(BatchedObject),
    .usage       = rhi::DeviceResourceState::StorageBufferRead | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_batched_objects_"
  });

  sbo_draw_commands_ = device.CreateBuffer(rhi::IBuffer::Info {
    .size        = kMaxMeshes * sizeof(rhi::DrawIndexedCommand),
    .usage       = rhi::DeviceResourceState::StorageBufferReadWrite | rhi::DeviceResourceState::IndirectArgument | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = "StaticMeshFeature::sbo_draw_commands_"
  });

  for (uint32_t object_idx = 0U; object_idx < kMaxObjects; ++object_idx) {
    free_list_.insert(object_idx);
  }
}

void StaticMeshFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  using namespace rhi;

  rg_versions_.staging_buffer = builder.DeclareTransientBuffer(IBuffer::Info {
    .size        = sbo_objects_->GetInfo().size + sbo_batched_objects_->GetInfo().size + sbo_draw_commands_->GetInfo().size,
    .usage       = DeviceResourceState::TransferSrc,
    .cpu_visible = true,
    .name        = "StaticMeshFeature - staging_buffer"
  });

  rg_versions_.objects         = builder.ImportBuffer(sbo_objects_.get(),         DeviceResourceState::TransferDst, DeviceResourceState::StorageBufferRead);
  rg_versions_.batched_objects = builder.ImportBuffer(sbo_batched_objects_.get(), DeviceResourceState::TransferDst, DeviceResourceState::StorageBufferRead);
  rg_versions_.draw_commands   = builder.ImportBuffer(sbo_draw_commands_.get(),   DeviceResourceState::TransferDst, DeviceResourceState::IndirectArgument);

  rg_versions_.visible_object_indices = builder.DeclareTransientBuffer(IBuffer::Info {
    .size        = kMaxObjects * sizeof(uint32_t),
    .usage       = DeviceResourceState::StorageBufferReadWrite,
    .cpu_visible = false,
    .name        = "StaticMeshFeature - sbo_visible_object_indices"
  });

  builder.BeginTransfer("StaticMeshFeature - Prepare Buffers");
  builder.ReadBuffer(rg_versions_.staging_buffer,   DeviceResourceState::TransferSrc);
  builder.WriteBuffer(rg_versions_.objects,         DeviceResourceState::TransferDst);
  builder.WriteBuffer(rg_versions_.batched_objects, DeviceResourceState::TransferDst);
  builder.WriteBuffer(rg_versions_.draw_commands,   DeviceResourceState::TransferDst);
  builder.SetJob([this](auto& graph, auto& context, auto& cmds) {
    bool prepare_draws_only = false;

    if (objects_added_ || !pending_remove_.empty()) {
      Rebuild(cmds);
      prepare_draws_only = false;
    }

    if (draw_commands_.empty()) {
      return;
    }

    auto staging_buffer = graph.GetBuffer(rg_versions_.staging_buffer);
    auto* staging_data = staging_buffer->MapMemory();

    uint64_t objects_data_size         = objects_.size() * sizeof(objects_[0U]);
    uint64_t batched_objects_data_size = batched_objects_.size() * sizeof(batched_objects_[0U]);
    uint64_t draw_commands_data_size   = draw_commands_.size() * sizeof(draw_commands_[0U]);

    uint64_t offset = 0U;
    if (!prepare_draws_only) {
      std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, objects_.data(), objects_data_size);
      cmds.CopyBuffer(staging_buffer, sbo_objects_.get(), objects_data_size, offset, 0U);
      offset += objects_data_size;
      std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, batched_objects_.data(), batched_objects_data_size);
      cmds.CopyBuffer(staging_buffer, sbo_batched_objects_.get(), batched_objects_data_size, offset, 0U);
      offset += batched_objects_data_size;
    }
    std::memcpy(reinterpret_cast<uint8_t*>(staging_data) + offset, draw_commands_.data(), draw_commands_data_size);
    cmds.CopyBuffer(staging_buffer, sbo_draw_commands_.get(), draw_commands_data_size, offset, 0U);

    staging_buffer->UnmapMemory();
  });
  builder.EndTransfer();

  builder.BeginCompute("StaticMeshFeature - Frustum Cull");
  builder.ReadBuffer(rg_versions_.objects,                 DeviceResourceState::StorageBufferRead);
  builder.ReadBuffer(rg_versions_.batched_objects,         DeviceResourceState::StorageBufferRead);
  builder.WriteBuffer(rg_versions_.visible_object_indices, DeviceResourceState::StorageBufferWrite);
  rg_versions_.final_draw_commands = builder.ReadWriteBuffer(rg_versions_.draw_commands, DeviceResourceState::StorageBufferReadWrite);
  builder.SetJob([this](auto& graph, auto& context, auto& cmds) {
    if (draw_commands_.empty()) {
      return;
    }

    const auto& camera_data = context.template Get<CameraData>();
    glm::mat4 proj_transposed = glm::transpose(camera_data.proj);
    glm::vec4 frustum_x = NormalizePlane(proj_transposed[3U] + proj_transposed[0U]);  // x + w < 0
    glm::vec4 frustum_y = NormalizePlane(proj_transposed[3U] + proj_transposed[1U]);  // y + w < 0
    glm::vec4 frustum   = glm::vec4(frustum_x.x, frustum_x.z, frustum_y.y, frustum_y.z);

    auto sbo_visible_object_indices = graph.GetBuffer(rg_versions_.visible_object_indices);
    cull_shader_->SetBuffer("BatchedObjects", sbo_batched_objects_->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("Draws", sbo_draw_commands_->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("Objects", sbo_objects_->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("VisibleObjectIndices", sbo_visible_object_indices->GetStorageDescriptorBinding());
    cull_shader_->SetBuffer("CameraData", context.template Get<CameraDataBinding>().binding_ubo);
    cull_shader_->SetPushConstant<glm::vec4>("frustum", frustum);
    cull_shader_->SetPushConstant<uint32_t>("batched_object_count", static_cast<uint32_t>(batched_objects_.size()));

    cull_shader_->BindPipeline(cmds);
    cull_shader_->BindPushConstants(cmds);
    cmds.Dispatch((batched_objects_.size() + 63U) / 64U, 1U, 1U);
  });
  builder.EndCompute();
}

void StaticMeshFeature::AddLayerJobs(LayerMap& layer_map) {
  auto& layer = layer_map["Opaque"];

  layer->Emplace([this](rhi::RenderGraphBuilder& builder) {
    builder.ReadBuffer(rg_versions_.objects,                rhi::DeviceResourceState::StorageBufferRead);
    builder.ReadBuffer(rg_versions_.visible_object_indices, rhi::DeviceResourceState::StorageBufferRead);
    builder.ReadBuffer(rg_versions_.final_draw_commands,    rhi::DeviceResourceState::IndirectArgument);
  });

  layer->Emplace([this](auto& graph, auto& context, auto& cmds) {
    if (draw_commands_.empty()) {
      return;
    }

    auto sbo_visible_object_indices = graph.GetBuffer(rg_versions_.visible_object_indices);
    render_shader_->SetBuffer("Objects", sbo_objects_->GetStorageDescriptorBinding());
    render_shader_->SetBuffer("VisibleObjectIndices", sbo_visible_object_indices->GetStorageDescriptorBinding());
    render_shader_->SetBuffer("CameraData", context.template Get<CameraDataBinding>().binding_ubo);

    render_shader_->BindPipeline(cmds);
    render_shader_->BindPushConstants(cmds);
    cmds.BindIndexBuffer(merged_index_buffer_.get());
    cmds.DrawIndexedIndirect(sbo_draw_commands_.get(), 0U, sizeof(draw_commands_[0]), draw_commands_.size());
  });
}

void StaticMeshFeature::SetupEntitySystems(ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void StaticMeshFeature::Run(const ecs::WorldTransform& transform, StaticMeshComponent& static_mesh) {
  if (static_mesh.mesh.GetState() != asset::State::Loaded) {
    return;
  }

  const uint32_t submeshes_count = static_mesh.mesh->submeshes.size();

  if (static_mesh.runtime_submesh_handles.size() != submeshes_count) {
    static_mesh.runtime_submesh_handles.resize(submeshes_count);

    for (uint32_t submesh_idx = 0U; submesh_idx < submeshes_count; ++submesh_idx) {
      const auto& submesh    = static_mesh.mesh->submeshes[submesh_idx];
      auto&       object_idx = static_mesh.runtime_submesh_handles[submesh_idx];
      if (submesh.material.GetState() != asset::State::Loaded) {
        continue;
      }

      object_idx = AddObject(Object {
        .binding_mesh     = submesh.ubo->GetUniformDescriptorBinding(),
        .binding_material = submesh.material->ubo->GetUniformDescriptorBinding(),
        .vertex_count     = submesh.vertex_count,
        .index_count      = submesh.index_count
      });

      index_buffers_per_object_[object_idx] = static_mesh.mesh->submeshes[submesh_idx].index_buffer.get();
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

void StaticMeshFeature::Rebuild(rhi::ICommandBuffer& cmds) {
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

  index_buffer_copies_.clear();
  merged_index_buffer_total_size_ = 0U;

  auto add_batch = [this](uint32_t from_idx, uint32_t batch_idx) {
    uint32_t object_idx = batched_objects_[from_idx].object_idx;

    uint64_t copy_size = index_buffers_per_object_[object_idx]->GetInfo().size;
    index_buffer_copies_.emplace_back(CopyCmd {
      .src        = index_buffers_per_object_[object_idx],
      .size       = copy_size,
      .dst_offset = merged_index_buffer_total_size_
    });

    uint32_t cur_first_index = merged_index_buffer_total_size_ / sizeof(uint32_t);
    merged_index_buffer_total_size_ += copy_size;

    draw_commands_.emplace_back(rhi::DrawIndexedCommand {
      .index_count    = objects_[object_idx].index_count,
      .instance_count = 0U,  // NOTE (tralf-strues): this value is computed in culling shader
      .first_index    = cur_first_index,
      .vertex_offset  = 0U,
      .first_instance = from_idx,
    });
  };

  uint32_t last_from_idx = 0U;
  uint32_t batch_idx     = 0U;
  for (uint32_t i = 0U; i < batched_objects_.size(); ++i) {
    if (i > 0 && objects_[batched_objects_[i - 1].object_idx].binding_mesh != objects_[batched_objects_[i].object_idx].binding_mesh) {
      add_batch(last_from_idx, batch_idx++);
      last_from_idx = i;
    }

    batched_objects_[i].batch_idx = batch_idx;
  }
  add_batch(last_from_idx, batch_idx);

  if (!merged_index_buffer_ || merged_index_buffer_->GetInfo().size > merged_index_buffer_total_size_) {
    merged_index_buffer_ = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = merged_index_buffer_total_size_,
      .usage       = rhi::DeviceResourceState::TransferDst | rhi::DeviceResourceState::IndexBuffer,
      .cpu_visible = false,
      .name        = "StaticMeshFeature::merged_index_buffer_"
    });
  }

  for (auto& copy : index_buffer_copies_) {
    cmds.CopyBuffer(copy.src, merged_index_buffer_.get(), copy.size, 0U, copy.dst_offset);
    cmds.BufferBarrier(merged_index_buffer_.get(), rhi::DeviceResourceState::TransferDst,
                       rhi::DeviceResourceState::IndexBuffer);
  }

  objects_added_ = false;
}

}  // namespace liger::render