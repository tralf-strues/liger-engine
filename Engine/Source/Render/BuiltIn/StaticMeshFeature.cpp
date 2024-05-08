// /**
//  * @author Nikita Mochalov (github.com/tralf-strues)
//  * @file StaticMeshFeature.cpp
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

// #include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

// namespace liger::render {

// StaticMeshFeature::StaticMeshFeature(rhi::IDevice& device, size_t reserve_capacity) {
//   const rhi::IBuffer::Info buffer_info {
//     .size = sizeof(),
//     .usage = ,
//     .cpu_visible = ,
//     .name = ,
//   };
//   transforms_buffer_ = device.CreateBuffer(const IBuffer::Info &info)
// }

// void StaticMeshFeature::SetupRenderTasks(liger::rhi::RenderGraphBuilder& builder) {}

// void StaticMeshFeature::SetupEntitySystems(liger::ecs::SystemGraph& systems) {}

// std::optional<shader::Declaration> StaticMeshFeature::GetShaderDeclaration() const {}

// void StaticMeshFeature::PreRender(rhi::IDevice&) {}

// }  // namespace liger::render