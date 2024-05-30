/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file TonemapFeature.cpp
 * @date 2024-05-29
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

#include <Liger-Engine/Render/BuiltIn/TonemapFeature.hpp>

#include <Liger-Engine/Render/BuiltIn/OutputTexture.hpp>

namespace liger::render {

TonemapFeature::TonemapFeature(asset::Manager& asset_manager, float exposure)
    : shader_(asset_manager.GetAsset<shader::Shader>(".liger/Shaders/BuiltIn.Tonemap.lshader")), exposure_(exposure) {}

void TonemapFeature::SetupRenderGraph(rhi::RenderGraphBuilder& builder) {
  const auto rg_src_color = builder.LastResourceVersion(builder.GetContext().Get<OutputTexture>().rg_hdr_color);
  const auto rg_dst_color = builder.GetContext().Get<OutputTexture>().rg_final_color;

  builder.BeginRenderPass("Tonemap");
  builder.SampleTexture(rg_src_color);
  builder.AddColorTarget(rg_dst_color, rhi::AttachmentLoad::Clear, rhi::AttachmentStore::Store);
  builder.SetJob([this, rg_src_color](rhi::RenderGraph& graph, auto& context, rhi::ICommandBuffer& cmds) {
    if (shader_.GetState() != asset::State::Loaded) {
      return;
    }

    auto src_texture = graph.GetTexture(rg_src_color);

    shader_->BindPipeline(cmds);
    shader_->SetPushConstant("exposure", exposure_);
    shader_->SetTextureSampler("SrcTexture", src_texture.texture->GetSampledDescriptorBinding());
    shader_->BindPushConstants(cmds);
    cmds.Draw(3U);
  });
  builder.EndRenderPass();
}

}  // namespace liger::render