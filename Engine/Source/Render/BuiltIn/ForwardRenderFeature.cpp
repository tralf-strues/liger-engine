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

namespace liger::render {

ForwardRenderFeature::ForwardRenderFeature(rhi::RenderGraph::ResourceVersion rg_color) : rg_color_(rg_color) {
  layers_.emplace_back(EnumToString(LayerType::Opaque));
  layers_.emplace_back(EnumToString(LayerType::Transparent));
}

void ForwardRenderFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  rhi::RenderGraph::DependentTextureInfo depth_info{};
  depth_info.extent.SetDependency(rg_color_);
  depth_info.format          = rhi::Format::D32_SFLOAT;
  depth_info.type            = rhi::TextureType::Texture2D;
  depth_info.usage           = rhi::DeviceResourceState::DepthStencilTarget;
  depth_info.cube_compatible = false;
  depth_info.mip_levels      = 1;
  depth_info.samples         = 1;
  depth_info.name            = "Depth buffer";
  rg_depth_ = builder.DeclareTransientTexture(depth_info);

  builder.BeginRenderPass("Forward Pass");
  builder.AddColorTarget(rg_color_, rhi::AttachmentLoad::Clear, rhi::AttachmentStore::Store);
  builder.SetDepthStencil(rg_depth_, rhi::AttachmentLoad::Clear, rhi::AttachmentStore::Discard);
  builder.SetJob([this](auto& graph, auto& context, rhi::ICommandBuffer& cmds) {
    for (auto& layer : layers_) {
      layer.Execute(graph, context, cmds);
    }
  });
  builder.EndRenderPass();
}

}  // namespace liger::render