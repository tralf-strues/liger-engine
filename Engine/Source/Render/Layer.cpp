/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Layer.cpp
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

#include <Liger-Engine/Render/Layer.hpp>

namespace liger::render {

Layer::Layer(std::string_view name) : name_(name) {}

std::string_view Layer::Name() const { return name_; }

void Layer::Emplace(Job job) {
  jobs_.emplace_back(std::move(job));
}

void Layer::Emplace(SetupTask setup_task) {
  setup_tasks_.emplace_back(std::move(setup_task));
}

void Layer::Setup(rhi::RenderGraphBuilder& builder) {
  for (auto& setup : setup_tasks_) {
    setup(builder);
  }
}

void Layer::Execute(rhi::RenderGraph& graph, rhi::Context& context, rhi::ICommandBuffer& cmds) {
  for (auto& job : jobs_) {
    job(graph, context, cmds);
  }
}

}  // namespace liger::render