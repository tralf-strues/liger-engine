/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file StaticMeshLoader.cpp
 * @date 2024-05-13
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

#include <Liger-Engine/Asset/Loaders/StaticMeshLoader.hpp>

#include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

namespace liger::asset::loaders {

template <typename T>
void BinaryRead(std::ifstream& is, T* data, uint32_t count = 1U) {
  is.read(reinterpret_cast<char*>(data), count * sizeof(T));
}

StaticMeshLoader::StaticMeshLoader(rhi::IDevice& device) : device_(device) {}

const std::filesystem::path& StaticMeshLoader::FileExtension() const {
  static std::filesystem::path extension{".lsmesh"};
  return extension;
}

void StaticMeshLoader::Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) {
  auto mesh = manager.GetAsset<render::StaticMesh>(asset_id);

  std::ifstream file(filepath, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filepath.string());
    mesh.UpdateState(asset::State::Invalid);
    return;
  }

  uint32_t submeshes_count;
  BinaryRead(file, &submeshes_count);

  rhi::IDevice::DedicatedTransferRequest transfer_request;

  for (uint32_t submesh_idx = 0U; submesh_idx < submeshes_count; ++submesh_idx) {
    uint32_t vertex_count;
    BinaryRead(file, &vertex_count);

    uint32_t index_count;
    BinaryRead(file, &index_count);

    const uint64_t vertex_buffer_size = vertex_count * sizeof(render::Vertex3D);
    const uint64_t index_buffer_size  = index_count * sizeof(uint32_t);

    auto vertices = std::make_unique<uint8_t[]>(vertex_count);
    BinaryRead(file, vertices.get(), vertex_buffer_size);
    
    auto indices = std::make_unique<uint8_t[]>(index_count);
    BinaryRead(file, indices.get(), index_buffer_size);

    glm::vec4 bounding_sphere;
    BinaryRead(file, &bounding_sphere);

    asset::Id material_id;
    BinaryRead(file, &material_id);

    render::Submesh submesh;
    submesh.vertex_count    = vertex_count;
    submesh.index_count     = index_count;
    submesh.bounding_sphere = bounding_sphere;
    submesh.material        = manager.GetAsset<render::Material>(material_id);

    submesh.ubo = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = sizeof(render::Submesh::UBO),
      .usage       = rhi::DeviceResourceState::UniformBuffer | rhi::DeviceResourceState::TransferDst,
      .cpu_visible = false,
      .name        = fmt::format("StaticMesh_0x{0:X}::submeshes[{1}]::ubo", asset_id.Value(), submesh_idx)
      });

    submesh.vertex_buffer = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = vertex_buffer_size,
      .usage       = rhi::DeviceResourceState::StorageBuffer | rhi::DeviceResourceState::TransferDst,
      .cpu_visible = false,
      .name        = fmt::format("StaticMesh_0x{0:X}::submeshes[{1}]::vertex_buffer", asset_id.Value(), submesh_idx)
    });

    submesh.index_buffer = device_.CreateBuffer(rhi::IBuffer::Info {
      .size        = index_buffer_size,
      .usage       = rhi::DeviceResourceState::StorageBuffer | rhi::DeviceResourceState::TransferDst,
      .cpu_visible = false,
      .name        = fmt::format("StaticMesh_0x{0:X}::submeshes[{1}]::index_buffer", asset_id.Value(), submesh_idx)
    });

    auto  ubo_data_raw             = std::make_unique<uint8_t[]>(sizeof(render::Submesh::UBO));
    auto& ubo_data                 = *reinterpret_cast<render::Submesh::UBO*>(ubo_data_raw.get());
    ubo_data.binding_vertex_buffer = submesh.vertex_buffer->GetStorageDescriptorBinding();
    ubo_data.binding_index_buffer  = submesh.index_buffer->GetStorageDescriptorBinding();
    ubo_data.vertex_count          = vertex_count;
    ubo_data.index_count           = index_count;
    ubo_data.bounding_sphere       = bounding_sphere;

    transfer_request.buffer_transfers.emplace_back(rhi::IDevice::DedicatedBufferTransfer {
      .buffer      = submesh.ubo.get(),
      .final_state = rhi::DeviceResourceState::UniformBuffer,
      .data        = std::move(ubo_data_raw),
      .size        = sizeof(render::Submesh::UBO),
    });

    transfer_request.buffer_transfers.emplace_back(rhi::IDevice::DedicatedBufferTransfer {
      .buffer      = submesh.vertex_buffer.get(),
      .final_state = rhi::DeviceResourceState::StorageBuffer,
      .data        = std::move(vertices),
      .size        = vertex_buffer_size,
    });

    transfer_request.buffer_transfers.emplace_back(rhi::IDevice::DedicatedBufferTransfer {
      .buffer      = submesh.index_buffer.get(),
      .final_state = rhi::DeviceResourceState::StorageBuffer,
      .data        = std::move(indices),
      .size        = index_buffer_size,
    });

    mesh->submeshes.emplace_back(std::move(submesh));
  }

  transfer_request.callback = [mesh]() mutable {
    mesh.UpdateState(asset::State::Loaded);
  };

  device_.RequestDedicatedTransfer(std::move(transfer_request));
}

}  // namespace liger::asset::loaders