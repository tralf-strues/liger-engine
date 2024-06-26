/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file SystemGraph.hpp
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

#pragma once

#include <Liger-Engine/ECS/System.hpp>

#include <unordered_set>

namespace liger::ecs {

class SystemGraph {
 public:
  void Emplace(std::unique_ptr<ISystem> system);
  void Insert(ISystem* system);

  tf::Taskflow Build(Scene& scene);

 private:
  using OwnedSystemStorage = std::vector<std::unique_ptr<ISystem>>;
  using SystemList         = std::vector<ISystem*>;
  using Graph              = std::vector<entt::organizer::vertex>;

  OwnedSystemStorage owned_systems_;
  SystemList         systems_;
  entt::organizer    organizer_;
  Graph              graph_;
};

}  // namespace liger::ecs