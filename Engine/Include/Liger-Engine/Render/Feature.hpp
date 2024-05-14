/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Feature.hpp
 * @date 2024-05-04
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

#include <Liger-Engine/ECS/SystemGraph.hpp>
#include <Liger-Engine/RHI/Device.hpp>
#include <Liger-Engine/RHI/RenderGraph.hpp>
#include <Liger-Engine/Render/Layer.hpp>
#include <Liger-Engine/ShaderSystem/DeclarationStack.hpp>

namespace liger::render {

class IFeature {
 public:
  virtual ~IFeature() = default;

  virtual std::string_view Name() const = 0;

  virtual std::span<const std::string_view> DependencyFeatures() const { return {}; }
  virtual std::span<Layer> Layers() { return {}; }

  virtual void SetupRenderGraph(rhi::RenderGraphBuilder&) {}

  virtual void SetupLayers(LayerMap&) {}

  virtual void SetupEntitySystems(ecs::SystemGraph&) {}

  virtual void PreRender(rhi::IDevice&, rhi::Context&) {}
  virtual void PostRender(rhi::IDevice&, rhi::Context&) {}
};

}  // namespace liger::render