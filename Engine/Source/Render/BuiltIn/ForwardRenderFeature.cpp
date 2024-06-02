/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ForwardRenderFeature.cpp
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

#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/Render/BuiltIn/ForwardRenderFeature.hpp>
#include <Liger-Engine/Render/BuiltIn/OutputTexture.hpp>

namespace liger::render {

ForwardRenderFeature::ForwardRenderFeature(rhi::RenderGraph::ResourceVersion rg_output) : rg_output_(rg_output) {
  layers_.emplace_back(EnumToString(LayerType::Opaque));
  layers_.emplace_back(EnumToString(LayerType::Transparent));
}

void ForwardRenderFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  rhi::RenderGraph::DependentTextureInfo color_info{};
  color_info.extent.SetDependency(rg_output_);
  color_info.format          = rhi::Format::B10G11R11_UFLOAT;
  color_info.type            = rhi::TextureType::Texture2D;
  color_info.usage           = rhi::DeviceResourceState::ColorTarget;
  color_info.cube_compatible = false;
  color_info.mip_levels      = 1;
  color_info.samples         = sample_count_;
  color_info.name            = "HDR Color Multisample";
  rg_color_ = builder.DeclareTransientTexture(color_info);

  rhi::RenderGraph::DependentTextureInfo color_resolve_info{};
  color_resolve_info.extent.SetDependency(rg_color_);
  color_resolve_info.format.SetDependency(rg_color_);
  color_resolve_info.type            = rhi::TextureType::Texture2D;
  color_resolve_info.usage           = rhi::DeviceResourceState::ColorMultisampleResolve | rhi::DeviceResourceState::ShaderSampled | rhi::DeviceResourceState::StorageTextureReadWrite;
  color_resolve_info.cube_compatible = false;
  color_resolve_info.mip_levels      = 1;
  color_resolve_info.samples         = 1;
  color_resolve_info.name            = "HDR Color";
  rg_resolve_ = builder.DeclareTransientTexture(color_resolve_info);

  rhi::RenderGraph::DependentTextureInfo depth_info{};
  depth_info.extent.SetDependency(rg_color_);
  depth_info.format          = rhi::Format::D32_SFLOAT;
  depth_info.type            = rhi::TextureType::Texture2D;
  depth_info.usage           = rhi::DeviceResourceState::DepthStencilTarget;
  depth_info.cube_compatible = false;
  depth_info.mip_levels      = 1;
  depth_info.samples.SetDependency(rg_color_);
  depth_info.name            = "Depth Multisample";
  rg_depth_ = builder.DeclareTransientTexture(depth_info);

  /* Opaque */
  builder.BeginRenderPass("Forward Pass - Opaque");

  rg_color_after_opaque_ = builder.AddColorTarget(rg_color_, rhi::AttachmentLoad::Clear, rhi::AttachmentStore::Store);
  rg_depth_after_opaque_ = builder.SetDepthStencil(rg_depth_, rhi::AttachmentLoad::Clear, rhi::AttachmentStore::Store);

  layers_[static_cast<size_t>(LayerType::Opaque)].Setup(builder);

  builder.SetJob([this](auto& graph, auto& context, rhi::ICommandBuffer& cmds) {
    layers_[static_cast<size_t>(LayerType::Opaque)].Execute(graph, context, cmds);
  });

  builder.EndRenderPass();

  /* Transparent */
  builder.BeginRenderPass("Forward Pass - Transparent");

  builder.AddColorTarget(rg_color_after_opaque_, rhi::AttachmentLoad::Load, rhi::AttachmentStore::Store);
  builder.AddColorMultisampleResolve(rg_resolve_);
  builder.SetDepthStencil(rg_depth_after_opaque_, rhi::AttachmentLoad::Load, rhi::AttachmentStore::Discard);

  layers_[static_cast<size_t>(LayerType::Transparent)].Setup(builder);

  builder.SetJob([this](auto& graph, auto& context, rhi::ICommandBuffer& cmds) {
    layers_[static_cast<size_t>(LayerType::Transparent)].Execute(graph, context, cmds);
  });

  builder.EndRenderPass();

  builder.GetContext().Insert(OutputTexture {
    .rg_hdr_color   = rg_resolve_,
    .rg_final_color = rg_output_
  });
}

void ForwardRenderFeature::UpdateSampleCount(uint8_t new_sample_count) {
  sample_count_ = new_sample_count;
}

void ForwardRenderFeature::PreRender(rhi::IDevice&, rhi::RenderGraph& graph, rhi::Context&) {
  graph.UpdateTransientTextureSamples(rg_color_, sample_count_);
}

}  // namespace liger::render