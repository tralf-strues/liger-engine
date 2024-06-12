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

#include <yaml-cpp/yaml.h>

namespace liger::asset::loaders {

MaterialLoader::MaterialLoader(rhi::IDevice& device) : device_(device) {}

std::span<const std::filesystem::path> MaterialLoader::FileExtensions() const {
  static std::array<std::filesystem::path, 1U> extension{".lmat"};
  return extension;
}

void MaterialLoader::Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) {
  auto material = manager.GetAsset<render::Material>(asset_id);

  auto root_node = YAML::LoadFile(filepath.string());
  if (!root_node) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filepath.string());
    material.UpdateState(asset::State::Invalid);
    return;
  }

  if (auto color_node = root_node["BaseColor"]; color_node) {
    for (uint32_t channel_idx = 0U; channel_idx < color_node.size(); ++channel_idx) {
      material->base_color[channel_idx] = color_node[channel_idx].as<float>();
    }
  }

  if (auto color_node = root_node["Emission"]; color_node) {
    for (uint32_t channel_idx = 0U; channel_idx < color_node.size(); ++channel_idx) {
      material->emission_color[channel_idx] = color_node[channel_idx].as<float>();
    }
  }

  if (auto emission_intensity_node = root_node["EmissionIntensity"]; emission_intensity_node) {
    material->emission_intensity = emission_intensity_node.as<float>();
  }

  if (auto metallic_node = root_node["Metallic"]; metallic_node) {
    material->metallic = metallic_node.as<float>();
  }

  if (auto roughness_node = root_node["Roughness"]; roughness_node) {
    material->roughness = roughness_node.as<float>();
  }

  if (auto base_color_map_node = root_node["BaseColorMap"]; base_color_map_node) {
    auto texture_id = base_color_map_node.as<uint64_t>();
    if (texture_id != asset::kInvalidId.Value()) {
      material->base_color_map = manager.GetAsset<std::unique_ptr<rhi::ITexture>>(asset::Id(texture_id));
    }
  }

  if (auto normal_map_node = root_node["NormalMap"]; normal_map_node) {
    auto texture_id = normal_map_node.as<uint64_t>();
    if (texture_id != asset::kInvalidId.Value()) {
      material->normal_map = manager.GetAsset<std::unique_ptr<rhi::ITexture>>(asset::Id(texture_id));
    }
  }

  if (auto metallic_roughness_map_node = root_node["MetallicRoughnessMap"]; metallic_roughness_map_node) {
    auto texture_id = metallic_roughness_map_node.as<uint64_t>();
    if (texture_id != asset::kInvalidId.Value()) {
      material->metallic_roughness_map = manager.GetAsset<std::unique_ptr<rhi::ITexture>>(asset::Id(texture_id));
    }
  }

  material->ubo = device_.CreateBuffer(rhi::IBuffer::Info {
    .size        = sizeof(render::Material::UBO),
    .usage       = rhi::DeviceResourceState::UniformBuffer | rhi::DeviceResourceState::TransferDst,
    .cpu_visible = false,
    .name        = fmt::format("Material_0x{0:X}::ubo", asset_id.Value())
  });

  auto  ubo_data_raw = std::make_unique<uint8_t[]>(sizeof(render::Material::UBO));
  auto& ubo_data     = *reinterpret_cast<render::Material::UBO*>(ubo_data_raw.get());

  ubo_data.base_color         = material->base_color;
  ubo_data.emission_color     = material->emission_color;
  ubo_data.emission_intensity = material->emission_intensity;
  ubo_data.metallic           = material->metallic;
  ubo_data.roughness          = material->roughness;

  ubo_data.binding_base_color_map = material->base_color_map
                                        ? material->base_color_map->get()->GetSampledDescriptorBinding()
                                        : rhi::TextureDescriptorBinding::Invalid;

  ubo_data.binding_normal_map = material->normal_map ? material->normal_map->get()->GetSampledDescriptorBinding()
                                                     : rhi::TextureDescriptorBinding::Invalid;

  ubo_data.binding_metallic_roughness_map = material->metallic_roughness_map
                                                ? material->metallic_roughness_map->get()->GetSampledDescriptorBinding()
                                                : rhi::TextureDescriptorBinding::Invalid;

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