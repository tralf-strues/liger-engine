/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file BloomFeature.cpp
 * @date 2024-05-28
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

#include <Liger-Engine/Render/BuiltIn/BloomFeature.hpp>
#include <Liger-Engine/Render/BuiltIn/OutputTexture.hpp>

namespace liger::render {

BloomFeature::BloomFeature(asset::Manager& asset_manager, Info info)
    : info_(std::move(info)), shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.Bloom.lshader")) {}

uint32_t CalcualteGroupCount(uint32_t resolution) {
  return (resolution + 31) / 32;
}

void BloomFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  rg_src_color_ = builder.LastResourceVersion(builder.GetContext().Get<OutputTexture>().rg_hdr_color);

  rhi::RenderGraph::DependentTextureInfo bloom_texture_info{};
  bloom_texture_info.extent.SetDependency(rg_src_color_);
  bloom_texture_info.format          = rhi::Format::B10G11R11_UFLOAT;
  bloom_texture_info.type            = rhi::TextureType::Texture2D;
  bloom_texture_info.usage           = rhi::DeviceResourceState::ShaderSampled | rhi::DeviceResourceState::StorageTextureWrite;
  bloom_texture_info.cube_compatible = false;
  bloom_texture_info.mip_levels      = info_.mip_count + 1U;
  bloom_texture_info.samples         = 1U;
  bloom_texture_info.name            = "Bloom Transient";
  rg_transient_ = builder.DeclareTransientTexture(bloom_texture_info);

  for (uint32_t mip = 1U; mip <= info_.mip_count; ++mip) {
    builder.DeclareTextureView(rg_transient_, rhi::TextureViewInfo {
      .type        = rhi::TextureViewType::View2D,
      .first_mip   = mip,
      .mip_count   = 1U,
      .first_layer = 0U,
      .layer_count = 1U
    });
  }

  constexpr uint32_t kTransientBaseMip = 1U;

  /* Main passes */
  builder.BeginCompute("Bloom");
  builder.SampleTexture(rg_src_color_);
  builder.WriteTexture(rg_transient_);
  builder.SetJob([this](rhi::RenderGraph& graph, auto& context, rhi::ICommandBuffer& cmds) {
    if (shader_.GetState() != asset::State::Loaded) {
      return;
    }

    auto transient = graph.GetTexture(rg_transient_);
    auto src_color = graph.GetTexture(rg_src_color_);

    const glm::vec4 prefilter_params = {
      info_.threshold,
      info_.threshold - info_.threshold * info_.soft_threshold,
      2.0f * info_.threshold * info_.soft_threshold,
      0.25f / (info_.threshold * info_.soft_threshold + 0.00001f)
    };

    const rhi::Extent3D original_resolution_3d = transient.texture->GetInfo().extent;
    const rhi::Extent2D original_resolution    = {original_resolution_3d.x, original_resolution_3d.y};

    shader_->BindPipeline(cmds);
    shader_->SetPushConstant("prefilter_params", prefilter_params);

    /* Prefilter */
    shader_->SetTextureSampler("SrcTexture", src_color.texture->GetSampledDescriptorBinding());
    shader_->SetTextureSampler("DstTexture", transient.texture->GetStorageDescriptorBinding(kTransientBaseMip));
    shader_->SetPushConstant("intensity",    info_.intensity);
    shader_->SetPushConstant("stage",        static_cast<uint32_t>(Stage::Prefilter));
    shader_->BindPushConstants(cmds);
    rhi::Extent2D cur_resolution = original_resolution.MipExtent(1U);
    cmds.Dispatch(CalcualteGroupCount(cur_resolution.x), CalcualteGroupCount(cur_resolution.y), 1U);

    /* Downsample */
    for (uint32_t mip = 0U; mip < info_.mip_count - 1; ++mip) {
      cmds.TextureBarrier(transient.texture,
                          rhi::JobType::Compute,
                          rhi::JobType::Compute,
                          rhi::DeviceResourceState::StorageTextureWrite,
                          rhi::DeviceResourceState::ShaderSampled,
                          kTransientBaseMip + mip);

      cmds.TextureBarrier(transient.texture,
                          rhi::JobType::Compute,
                          rhi::JobType::Compute,
                          rhi::DeviceResourceState::Undefined,
                          rhi::DeviceResourceState::StorageTextureWrite,
                          kTransientBaseMip + mip + 1U);

      shader_->SetTextureSampler("SrcTexture", transient.texture->GetSampledDescriptorBinding(kTransientBaseMip + mip));
      shader_->SetTextureSampler("DstTexture", transient.texture->GetStorageDescriptorBinding(kTransientBaseMip + mip + 1U));
      shader_->SetPushConstant("intensity",    info_.intensity);
      shader_->SetPushConstant("stage",        static_cast<uint32_t>(Stage::Downsample));
      shader_->BindPushConstants(cmds);
      cur_resolution = original_resolution.MipExtent(kTransientBaseMip + mip + 1U);
      cmds.Dispatch(CalcualteGroupCount(cur_resolution.x), CalcualteGroupCount(cur_resolution.y), 1U);
    }

    /* Upsample */
    for (uint32_t mip = info_.mip_count - 1; mip > 0; --mip) {
      cmds.TextureBarrier(transient.texture,
                          rhi::JobType::Compute,
                          rhi::JobType::Compute,
                          rhi::DeviceResourceState::StorageTextureWrite,
                          rhi::DeviceResourceState::ShaderSampled,
                          kTransientBaseMip + mip);

      cmds.TextureBarrier(transient.texture,
                          rhi::JobType::Compute,
                          rhi::JobType::Compute,
                          rhi::DeviceResourceState::ShaderSampled,
                          rhi::DeviceResourceState::StorageTextureWrite,
                          kTransientBaseMip + mip - 1U);

      shader_->SetTextureSampler("SrcTexture", transient.texture->GetSampledDescriptorBinding(kTransientBaseMip + mip));
      shader_->SetTextureSampler("DstTexture", transient.texture->GetStorageDescriptorBinding(kTransientBaseMip + mip - 1U));
      shader_->SetPushConstant("intensity",    info_.intensity);
      shader_->SetPushConstant("stage",        static_cast<uint32_t>(Stage::Upsample));
      shader_->BindPushConstants(cmds);
      cur_resolution = original_resolution.MipExtent(kTransientBaseMip + mip - 1U);
      cmds.Dispatch(CalcualteGroupCount(cur_resolution.x), CalcualteGroupCount(cur_resolution.y), 1U);
    }

    // FIXME (tralf-strues)
    for (uint32_t mip = 1U; mip < info_.mip_count; ++mip) {
      cmds.TextureBarrier(transient.texture,
                          rhi::JobType::Compute,
                          rhi::JobType::Compute,
                          rhi::DeviceResourceState::ShaderSampled,
                          rhi::DeviceResourceState::StorageTextureWrite,
                          kTransientBaseMip + mip);
    }

    //cmds.TextureBarrier(transient.texture,
    //                    rhi::JobType::Compute,
    //                    rhi::JobType::Compute,
    //                    rhi::DeviceResourceState::Undefined,
    //                    rhi::DeviceResourceState::StorageTextureWrite,
    //                    0U);
  });
  builder.EndCompute();

  /* Compose */
  builder.BeginCompute("Bloom Compose");
  builder.SampleTexture(rg_transient_);
  rg_dst_color_ = builder.ReadWriteTexture(rg_src_color_);
  builder.SetJob([this](rhi::RenderGraph& graph, auto& context, rhi::ICommandBuffer& cmds) {
    if (shader_.GetState() != asset::State::Loaded) {
      return;
    }

    auto transient = graph.GetTexture(rg_transient_);
    auto src_color = graph.GetTexture(rg_src_color_);

    shader_->BindPipeline(cmds);
    shader_->SetTextureSampler("SrcTexture", transient.texture->GetSampledDescriptorBinding(kTransientBaseMip));
    shader_->SetTextureSampler("DstTexture", src_color.texture->GetStorageDescriptorBinding());
    shader_->SetPushConstant("intensity",    info_.intensity);
    shader_->SetPushConstant("stage",        static_cast<uint32_t>(Stage::Compose));
    shader_->BindPushConstants(cmds);
    auto cur_resolution = src_color.texture->GetInfo().extent;
    cmds.Dispatch(CalcualteGroupCount(cur_resolution.x), CalcualteGroupCount(cur_resolution.y), 1U);
  });
  builder.EndCompute();
}

void BloomFeature::UpdateInfo(const Info& info) {
  info_ = info;
}

}  // namespace liger::render