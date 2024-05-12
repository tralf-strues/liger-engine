/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file CameraMovementScript.cpp
 * @date 2024-05-11
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

#include <Liger-Engine/Core/Platform/PlatformLayer.hpp>
#include <Liger-Engine/ECS/BuiltIn/CameraMovementScript.hpp>
#include <Liger-Engine/ECS/DefaultComponents.hpp>

namespace liger::ecs {

CameraMovementScript::CameraMovementScript(Window* window) : window_(window) {}

void CameraMovementScript::OnAttach(Entity entity) {
  PlatformLayer::Instance().GetSink<MouseMoveEvent>().Connect<&CameraMovementScript::OnMouseMove>(*this);
  PlatformLayer::Instance().GetSink<MouseButtonEvent>().Connect<&CameraMovementScript::OnMouseButton>(*this);
}

void CameraMovementScript::OnUpdate(entt::registry& registry, Entity entity, float dt) {
  auto* transform = registry.try_get<WorldTransform>(entity);
  if (!transform) {
    return;
  } 

  glm::vec3 forward = transform->Forward();
  glm::vec3 right   = transform->Right();
  glm::vec3 up      = transform->Up();

  float disp = kSpeed * dt;

  if (PlatformLayer::Instance().KeyPressed(window_, Key::W)) {
    transform->position += disp * forward;
  } else if (PlatformLayer::Instance().KeyPressed(window_, Key::S)) {
    transform->position -= disp * forward;
  }

  if (PlatformLayer::Instance().KeyPressed(window_, Key::D)) {
    transform->position += disp * right;
  } else if (PlatformLayer::Instance().KeyPressed(window_, Key::A)) {
    transform->position -= disp * right;
  }

  if (PlatformLayer::Instance().KeyPressed(window_, Key::E)) {
    transform->position += 0.5f * disp * up;
  } else if (PlatformLayer::Instance().KeyPressed(window_, Key::Q)) {
    transform->position -= 0.5f * disp * up;
  }

  transform->rotation = glm::quat(glm::vec3(rotation_z_, rotation_y_, 0));
  //transform->position.y = std::max(2.5f, transform->position.y);
}

bool CameraMovementScript::OnMouseMove(const MouseMoveEvent& mouse_move) {
  if (!rotation_mode_) {
    return true;
  }

  rotation_y_ -= 0.001f * mouse_move.delta.x;
  rotation_z_ -= 0.001f * mouse_move.delta.y;

  return true;
}

bool CameraMovementScript::OnMouseButton(const MouseButtonEvent& mouse_button) {
  if (mouse_button.button == MouseButton::Right) {
    if (mouse_button.action == PressAction::Press) {
      rotation_mode_ = true;
      PlatformLayer::Instance().SetCursorEnabled(window_, false);
    } else if (mouse_button.action == PressAction::Release) {
      rotation_mode_ = false;
      PlatformLayer::Instance().SetCursorEnabled(window_, true);
    }
  }

  return true;
}

}  // namespace liger::ecs