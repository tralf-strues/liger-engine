/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Shader.cpp
 * @date 2024-04-16
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

#include <Liger-Engine/ShaderSystem/Shader.hpp>

#include <iostream>

namespace liger::shader {

Shader::~Shader() {
  delete[] push_constant_data_;
  delete[] property_buffer_data_;
}

void Shader::SetTextureSampler(std::string_view name, rhi::TextureDescriptorBinding binding) {
  SetPushConstant(name, binding);
}

void Shader::SetBuffer(std::string_view name, rhi::BufferDescriptorBinding binding) {
  SetPushConstant(name, binding);
}

void Shader::BindPipeline(rhi::ICommandBuffer& cmds) const {
  cmds.BindPipeline(pipeline_.get());
}

void Shader::BindPushConstants(rhi::ICommandBuffer& cmds) const {
  cmds.SetPushConstant(pipeline_.get(), std::span<const char>(push_constant_data_, push_constant_size_));
}

}  // namespace liger::shader