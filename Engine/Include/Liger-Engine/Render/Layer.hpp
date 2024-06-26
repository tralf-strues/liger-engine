/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Layer.hpp
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

#pragma once

#include <Liger-Engine/RHI/Device.hpp>

#include <concepts>

namespace liger::render {

class Layer {
 public:
  using SetupTask = std::function<void(rhi::RenderGraphBuilder&)>;
  using Job       = rhi::RenderGraph::Job;

  explicit Layer(std::string_view name);

  std::string_view Name() const;

  void Emplace(Job job);
  void Emplace(SetupTask setup_task);

  void Setup(rhi::RenderGraphBuilder& builder);
  void Execute(rhi::RenderGraph& graph, rhi::Context& context, rhi::ICommandBuffer& cmds);

 private:
  std::string            name_;
  std::vector<Job>       jobs_;
  std::vector<SetupTask> setup_tasks_;
};

using LayerMap = std::unordered_map<std::string_view, Layer*>;

}  // namespace liger::render