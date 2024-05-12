/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Renderer.cpp
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

#include <Liger-Engine/Render/Renderer.hpp>

namespace liger::render {

Renderer::Renderer(rhi::IDevice& device) : device_(device), rg_builder_(device_.NewRenderGraphBuilder()) {}

void Renderer::EmplaceFeature(std::unique_ptr<IFeature> feature) {
  features_.emplace_back(std::move(feature));
}

void Renderer::Setup() {
  LayerMap layers;
  for (auto& feature : features_) {
    for (auto& layer : feature->Layers()) {
      layers[layer.Name()] = &layer;
    }
  }

  for (auto& feature : features_) {
    feature->SetupRenderGraph(rg_builder_);
  }

  render_graph_ = rg_builder_.Build(device_, "Renderer::render_graph_");

  for (auto& feature : features_) {
    feature->LinkRenderJobs(*render_graph_);
    feature->SetupLayerJobs(layers);
  }

  for (auto& feature : features_) {
    feature->SetupEntitySystems(system_graph_);
  }
}

rhi::RenderGraphBuilder& Renderer::GetRenderGraphBuilder() { return rg_builder_; }

tf::Taskflow Renderer::GetSystemTaskflow(ecs::Scene& scene) { return system_graph_.Build(scene); }

rhi::RenderGraph& Renderer::GetRenderGraph() { return *render_graph_; }

void Renderer::Render() {
  for (auto& feature : features_) {
    feature->PreRender(device_);
  }

  device_.ExecuteConsecutive(*render_graph_);

  for (auto& feature : features_) {
    feature->PostRender(device_);
  }
}

}  // namespace liger::render