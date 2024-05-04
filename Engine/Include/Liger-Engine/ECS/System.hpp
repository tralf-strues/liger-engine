/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file System.hpp
 * @date 2024-04-30
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

#include <Liger-Engine/ECS/Scene.hpp>

namespace liger::ecs {

class ISystem {
 public:
  virtual ~ISystem() = default;

  virtual void Setup(entt::organizer& organizer) = 0;
  virtual void Prepare(entt::registry& registry) = 0;

  virtual void RunForEach(entt::registry& registry) = 0;

  virtual std::string_view Name() const = 0;
};

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
class ComponentSystem : public ISystem {
 public:
  ~ComponentSystem() override = default;

  void Setup(entt::organizer& organizer) final;
  void Prepare(entt::registry& registry) final;
  void RunForEach(entt::registry& registry) final;

  virtual void Run(Components&... components) = 0;
};

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
void ComponentSystem<Components...>::Setup(entt::organizer& organizer) {
  using SystemT = ComponentSystem<Components...>;
  organizer.emplace<&SystemT::Run>(*this, Name().data());
}

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
void ComponentSystem<Components...>::Prepare(entt::registry& registry) {
  registry.view<Components...>();
}

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
void ComponentSystem<Components...>::RunForEach(entt::registry& registry) {
  registry.view<Components...>().each([this](Components&... components) {
    Run(components...);
  });
}

}  // namespace liger::ecs