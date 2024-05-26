/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DefaultComponents.hpp
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

#include <Liger-Engine/Core/Math/Math.hpp>
#include <Liger-Engine/ECS/Scene.hpp>
#include <Liger-Engine/ECS/Script.hpp>

#include <string>

namespace liger::ecs {

struct NameComponent {
  std::string name;

  explicit NameComponent(std::string_view name = "") : name(name) {}
};

struct WorldTransform : Transform3D {};

struct Camera {
  float fov          {60.0f};
  float near         {0.1f};
  float far          {250.0f};
  float aspect       {1.0f};
  bool  fixed_aspect {false};

  inline glm::mat4 ProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspect, near, far);
  }
};

struct ScriptComponent {
  ScriptComponent() = default;
  ScriptComponent(std::unique_ptr<IScript> script) : script(std::move(script)) {}

  std::unique_ptr<IScript> script;
};

}  // namespace liger::ecs