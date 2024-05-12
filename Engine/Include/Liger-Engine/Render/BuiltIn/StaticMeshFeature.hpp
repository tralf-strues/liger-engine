// /**
//  * @author Nikita Mochalov (github.com/tralf-strues)
//  * @file StaticMeshFeature.hpp
//  * @date 2024-05-05
//  *
//  * The MIT License (MIT)
//  * Copyright (c) 2023 Nikita Mochalov
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a
//  * copy of this software and associated documentation files (the "Software"),
//  * to deal in the Software without restriction, including without limitation
//  * the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  * and/or sell copies of the Software, and to permit persons to whom the
//  * Software is furnished to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in
//  * all copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  * DEALINGS IN THE SOFTWARE.
//  */

// #pragma once

// #include <Liger-Engine/ECS/DefaultComponents.hpp>
// #include <Liger-Engine/Render/Feature.hpp>

// namespace liger::render {

// struct StaticMeshComponent {
  
// };

// class StaticMeshFeature : public IFeature, ecs::ComponentSystem<ecs::WorldTransform> {
//  public:
//   struct Vertex3D {
//     glm::vec3 position;
//     glm::vec3 normal;
//     glm::vec3 tangent;
//     glm::vec2 tex_coords;
//   };

//   explicit StaticMeshFeature(rhi::IDevice& device, size_t reserve_capacity = 2048);
//   ~StaticMeshFeature() override = default;

//   std::string_view Name() const override { return "StaticMeshFeature"; }

//   void SetupRenderTasks(liger::rhi::RenderGraphBuilder& builder) override;
//   void SetupEntitySystems(liger::ecs::SystemGraph& systems) override;
//   std::optional<shader::Declaration> GetShaderDeclaration() const override;

//   void PreRender(rhi::IDevice&) override;

//  private:
//   std::unique_ptr<rhi::IBuffer> transforms_buffer_;
// };

// }  // namespace liger::render