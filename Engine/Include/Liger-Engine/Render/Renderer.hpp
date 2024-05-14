/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Renderer.hpp
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

#pragma once

#include <Liger-Engine/Render/Feature.hpp>

namespace liger::render {

class Renderer {
 public:
  using FeatureList        = std::vector<std::unique_ptr<IFeature>>;
  using DeclarationLibrary = std::unordered_map<std::string_view, shader::Declaration>;

  explicit Renderer(rhi::IDevice& device);

  void EmplaceFeature(std::unique_ptr<IFeature> feature);
  rhi::RenderGraphBuilder& GetRenderGraphBuilder();

  void Setup();

  tf::Taskflow GetSystemTaskflow(ecs::Scene& scene);
  rhi::RenderGraph& GetRenderGraph();

  void Render();

 private:
  rhi::IDevice&                     device_;

  FeatureList                       features_;
  DeclarationLibrary                declarations_;
  ecs::SystemGraph                  system_graph_;

  rhi::RenderGraphBuilder           rg_builder_;
  std::unique_ptr<rhi::RenderGraph> render_graph_;
  rhi::Context                      context_;
};

}  // namespace liger::render