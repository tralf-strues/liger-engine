/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file TextureLoader.cpp
 * @date 2024-05-17
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

#include <Liger-Engine/Asset/Loaders/TextureLoader.hpp>

#include <Liger-Engine/Asset/LogChannel.hpp>
#include <Liger-Engine/Asset/Manager.hpp>
#include <Liger-Engine/RHI/Device.hpp>
#include <Liger-Engine/RHI/Texture.hpp>

#include <stb_image/stb_image.h>

namespace liger::asset::loaders {

TextureLoader::TextureLoader(rhi::IDevice& device) : device_(device) {}

std::span<const std::filesystem::path> TextureLoader::FileExtensions() const {
  static std::array<std::filesystem::path, 2U> extensions = {".jpg", ".png"};
  return extensions;
}

void TextureLoader::Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) {
  auto texture = manager.GetAsset<std::unique_ptr<rhi::ITexture>>(asset_id);

  int32_t tex_width    = 0;
  int32_t tex_height   = 0;
  int32_t tex_channels = 0;

  int32_t info_result = stbi_info(filepath.string().c_str(), &tex_width, &tex_height, &tex_channels);
  if (!info_result) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Texture file '{}' not found!", filepath.string());
    return;
  }

  rhi::Format format;
  int32_t     desired_channels;
  switch (tex_channels) {
    case 1: {
      format           = rhi::Format::R8_UNORM;
      desired_channels = STBI_grey;
      break;
    }
    case 3:
    case 4: {
      format           = rhi::Format::R8G8B8A8_UNORM;
      desired_channels = STBI_rgb_alpha;
      break;
    }
    default: {
      LIGER_LOG_FATAL(kLogChannelAsset, "Unsupported number of channels: {0}", tex_channels);
      return;
    }
  }

  stbi_set_flip_vertically_on_load(true);
  stbi_uc* pixels = stbi_load(filepath.string().c_str(), &tex_width, &tex_height, &tex_channels, desired_channels);
  if (!pixels) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Texture file '{}' not found!", filepath.string());
    return;
  }

  uint32_t tex_mip_levels     = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1U;
  uint32_t texture_size_bytes = tex_width * tex_height * static_cast<uint32_t>(desired_channels);

  std::unique_ptr<uint8_t[]> raw_data = std::make_unique<uint8_t[]>(texture_size_bytes);
  std::memcpy(raw_data.get(), pixels, texture_size_bytes);
  stbi_image_free(pixels);

  (*texture) = device_.CreateTexture(rhi::ITexture::Info {
    .format          = format,
    .type            = rhi::TextureType::Texture2D,
    .usage           = rhi::DeviceResourceState::ShaderSampled | rhi::DeviceResourceState::TransferSrc | rhi::DeviceResourceState::TransferDst,
    .cube_compatible = false,
    .extent          = {.x = static_cast<uint32_t>(tex_width), .y = static_cast<uint32_t>(tex_height), .z = 1},
    .mip_levels      = tex_mip_levels,
    .samples         = 1U,
    .name            = fmt::format("Texture_0x{0:X}({1})", asset_id.Value(), filepath.stem().string())
  });

  const rhi::SamplerInfo sampler_info {
    .min_filter         = rhi::Filter::Linear,
    .mag_filter         = rhi::Filter::Linear,
    .address_mode_u     = rhi::SamplerInfo::AddressMode::Repeat,
    .address_mode_v     = rhi::SamplerInfo::AddressMode::Repeat,
    .address_mode_w     = rhi::SamplerInfo::AddressMode::Repeat,
    .border_color       = rhi::SamplerInfo::BorderColor::IntOpaqueBlack,
    .anisotropy_enabled = true,
    .max_anisotropy     = 4.0f,
    .mipmap_mode        = rhi::Filter::Linear,
    .min_lod            = 0.0f,
    .max_lod            = rhi::SamplerInfo::kMaxLODClampNone,
    .lod_bias           = 0.0f
  };

  (*texture)->SetSampler(sampler_info, rhi::kTextureDefaultViewIdx);

  rhi::IDevice::DedicatedTransferRequest transfer_request;
  transfer_request.texture_transfers.emplace_back(rhi::IDevice::DedicatedTextureTransfer {
    .texture         = texture->get(),
    .final_state     = rhi::DeviceResourceState::ShaderSampled,
    .data            = std::move(raw_data),
    .size            = texture_size_bytes,
    .gen_mips        = true,
    .gen_mips_filter = rhi::Filter::Linear
  });
  transfer_request.callback = [texture]() mutable {
    texture.UpdateState(asset::State::Loaded);
  };

  device_.RequestDedicatedTransfer(std::move(transfer_request));
}

}  // namespace liger::asset::loaders