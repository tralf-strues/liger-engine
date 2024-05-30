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

  virtual void Setup(entt::registry& registry) {}

  virtual void SetupExecution(entt::organizer& organizer) = 0;
  virtual void PrepareRegistry(entt::registry& registry) = 0;

  virtual void RunForEach(entt::registry& registry) = 0;

  virtual std::string_view Name() const = 0;
};

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
class ComponentSystem : public ISystem {
 public:
  ~ComponentSystem() override = default;

  virtual void Run(Components&... components) = 0;

  void SetupExecution(entt::organizer& organizer) final {
    using SystemT = ComponentSystem<Components...>;
    organizer.emplace<&SystemT::Run>(*this, Name().data());
  }

  void PrepareRegistry(entt::registry& registry) final {
    [[maybe_unused]] auto view = registry.view<Components...>();
  }

  void RunForEach(entt::registry& registry) final {
    registry.view<Components...>().each([this](Components&... components) {
      Run(components...);
    });
  }
};

template <typename... Components>
requires (!std::is_empty_v<Components> && ...)
class ExclusiveComponentSystem : public ISystem {
 public:
  ~ExclusiveComponentSystem() override = default;

  virtual void Run(entt::registry& registry, entt::entity entity, Components&... components) = 0;

  void SetupExecution(entt::organizer& organizer) final {
    using SystemT = ExclusiveComponentSystem<Components...>;
    organizer.emplace<&SystemT::Run>(*this, Name().data());
  }

  void PrepareRegistry(entt::registry& registry) final {
    [[maybe_unused]] auto view = registry.view<Components...>();
  }

  void RunForEach(entt::registry& registry) final {
    registry.view<Components...>().each([this, &registry](entt::entity entity, Components&... components) {
      Run(registry, entity, components...);
    });
  }
};

}  // namespace liger::ecs