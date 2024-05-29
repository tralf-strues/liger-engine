/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file BloomFeature.hpp
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

#pragma once

#include <Liger-Engine/Asset/Manager.hpp>
#include <Liger-Engine/ECS/DefaultComponents.hpp>
#include <Liger-Engine/RHI/ShaderAlignment.hpp>
#include <Liger-Engine/Render/Feature.hpp>
#include <Liger-Engine/ShaderSystem/Shader.hpp>

namespace liger::render {

class BloomFeature : public IFeature {
 public:
  struct Info {
    uint32_t mip_count      = 6U;
    float    threshold      = 1.0f;
    float    soft_threshold = 0.5f;
    float    intensity      = 1.0f;
  };

  explicit BloomFeature(asset::Manager& asset_manager, Info info);
  ~BloomFeature() override = default;

  std::string_view Name() const override { return "BloomFeature"; }

  void SetupRenderGraph(rhi::RenderGraphBuilder& builder) override;

  void UpdateInfo(const Info& info);

 private:
  enum class Stage : uint32_t {
    Prefilter  = 0U,
    Downsample = 1U,
    Upsample   = 2U,
    Compose    = 3U
  };

  Info                              info_;
  asset::Handle<shader::Shader>     shader_;

  rhi::RenderGraph::ResourceVersion rg_src_color_;
  rhi::RenderGraph::ResourceVersion rg_dst_color_;

  rhi::RenderGraph::ResourceVersion rg_transient_;
};

}  // namespace liger::render