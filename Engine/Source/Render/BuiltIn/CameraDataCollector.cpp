/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file CameraDataCollector.hpp
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

#include <Liger-Engine/Render/BuiltIn/CameraDataCollector.hpp>

namespace liger::render {

CameraDataCollector::CameraDataCollector(rhi::IDevice& device)
    : ubo_camera_data_(device, rhi::DeviceResourceState::UniformBuffer, "CameraDataCollector::ubo_camera_data_", 1U) {}

void CameraDataCollector::SetupEntitySystems(ecs::SystemGraph& systems) {
  systems.Insert(this);
}

void CameraDataCollector::Run(const ecs::Camera& camera, const ecs::WorldTransform& transform) {
  auto* data        = ubo_camera_data_.GetData();
  data->view        = transform.InverseMatrix();
  data->proj        = camera.ProjectionMatrix();
  data->ws_position = transform.position;
  data->near        = camera.near;
  data->far         = camera.far;
}

void CameraDataCollector::PreRender(rhi::IDevice&, rhi::Context& context) {
  context.Insert(*ubo_camera_data_.GetData());
  context.Insert(CameraDataBinding(ubo_camera_data_->GetUniformDescriptorBinding()));
}

}  // namespace liger::render