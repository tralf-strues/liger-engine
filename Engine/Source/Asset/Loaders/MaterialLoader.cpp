/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file MaterialLoader.cpp
 * @date 2024-05-14
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

#include <Liger-Engine/Asset/Loaders/MaterialLoader.hpp>

#include <Liger-Engine/Asset/LogChannel.hpp>
#include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

namespace liger::asset::loaders {

MaterialLoader::MaterialLoader(rhi::IDevice& device) : device_(device) {}

const std::filesystem::path& MaterialLoader::FileExtension() const {
  static std::filesystem::path extension{".lmat"};
  return extension;
}

void MaterialLoader::Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) {
  auto material = manager.GetAsset<render::Material>(asset_id);

  std::ifstream file(filepath, std::ios::in);
  if (!file.is_open()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filepath.string());
    material.UpdateState(asset::State::Invalid);
    return;
  }

  material->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

  material->ubo = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = sizeof(render::Material::UBO),
    .usage       = rhi::DeviceResourceState::UniformBuffer | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = fmt::format("Material_0x{0:X}::ubo", asset_id.Value())
  });

  auto  ubo_data_raw = std::make_unique<uint8_t[]>(sizeof(render::Material::UBO));
  auto& ubo_data     = *reinterpret_cast<render::Material::UBO*>(ubo_data_raw.get());
  ubo_data.color     = material->color;

  rhi::IDevice::DedicatedTransferRequest transfer_request;
  transfer_request.buffer_transfers.emplace_back(rhi::IDevice::DedicatedBufferTransfer {
    .buffer      = material->ubo.get(),
    .final_state = rhi::DeviceResourceState::UniformBuffer,
    .data        = std::move(ubo_data_raw),
    .size        = sizeof(render::Material::UBO),
  });
  transfer_request.callback = [material]() mutable {
    material.UpdateState(asset::State::Loaded);
  };

  device_.RequestDedicatedTransfer(std::move(transfer_request));
}

}  // namespace liger::asset::loaders