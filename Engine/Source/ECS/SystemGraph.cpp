/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file SystemGraph.cpp
 * @date 2024-05-01
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

#include <Liger-Engine/ECS/SystemGraph.hpp>

#include <Liger-Engine/ECS/LogChannel.hpp>

#include <algorithm>

namespace liger::ecs {

void SystemGraph::Insert(std::unique_ptr<ISystem> system) {
  system->Setup(organizer_);
  systems_.emplace_back(std::move(system));
}

tf::Taskflow SystemGraph::Build(Scene& scene) {
  tf::Taskflow taskflow;
  graph_ = organizer_.graph();

  std::vector<tf::Task> tasks;
  tasks.reserve(graph_.size());

  // NOLINTNEXTLINE(modernize-loop-convert)
  for (uint32_t node_idx = 0U; node_idx < graph_.size(); ++node_idx) {
    auto task = taskflow.emplace([this, &scene, node_idx]() {
      auto& system = const_cast<ISystem&>(*reinterpret_cast<const ISystem*>(graph_[node_idx].data()));
      system.RunForEach(scene.GetRegistry());
    });

    task.name(graph_[node_idx].name());
    tasks.emplace_back(task);
  }

  for (uint32_t node_idx = 0U; node_idx < graph_.size(); ++node_idx) {
    for (uint32_t to_idx : graph_[node_idx].children()) {
      tasks[node_idx].precede(tasks[to_idx]);
    }
  }

  for (auto& system : systems_) {
    system->Prepare(scene.GetRegistry());
  }

  return taskflow;
}

}  // namespace liger::ecs