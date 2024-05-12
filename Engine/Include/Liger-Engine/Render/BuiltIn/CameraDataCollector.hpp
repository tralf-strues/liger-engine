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

#pragma once

#include <Liger-Engine/ECS/DefaultComponents.hpp>
#include <Liger-Engine/RHI/MappedBuffer.hpp>
#include <Liger-Engine/RHI/ShaderAlignment.hpp>
#include <Liger-Engine/Render/Feature.hpp>

namespace liger::render {

class CameraDataCollector : public IFeature, public ecs::ComponentSystem<const ecs::Camera, const ecs::WorldTransform> {
 public:
  struct Data {
    SHADER_STRUCT_MEMBER(glm::mat4) view;
    SHADER_STRUCT_MEMBER(glm::mat4) proj;
    SHADER_STRUCT_MEMBER(glm::vec3) ws_position;
    SHADER_STRUCT_MEMBER(float)     near;
    SHADER_STRUCT_MEMBER(float)     far;
  };

  explicit CameraDataCollector(rhi::IDevice& device);
  ~CameraDataCollector() override = default;

  std::string_view Name() const override { return "CameraDataCollector<const Camera, const WorldTransform>"; }

  const Data& GetData() const;
  rhi::BufferDescriptorBinding GetBufferBinding() const;

  void SetupEntitySystems(ecs::SystemGraph& systems) override;
  void Run(const ecs::Camera& camera, const ecs::WorldTransform& transform) override;

 private:
  rhi::UniqueMappedBuffer<Data> ubo_camera_data_;
};

}  // namespace liger::render