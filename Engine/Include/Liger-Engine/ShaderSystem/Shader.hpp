/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file shader.hpp
 * @date 2024-04-15
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

#include <Liger-Engine/RHI/Device.hpp>

namespace liger::shader {

struct Declaration;

class Shader {
 public:
  ~Shader();

  void SetTextureSampler(std::string_view name, rhi::TextureDescriptorBinding binding);
  void SetBuffer(std::string_view name, rhi::BufferDescriptorBinding binding);

  template <typename T>
  void SetProperty(std::string_view name, T value);

  template <typename T>
  void SetPushConstant(std::string_view name, T value);

  void Bind(rhi::ICommandBuffer& cmd) const;

 private:
  Shader() = default;

  std::unique_ptr<rhi::IPipeline>           pipeline_{nullptr};

  char*                                     push_constant_data_{nullptr};
  uint32_t                                  push_constant_size_{0};
  std::unordered_map<std::string, uint32_t> push_constant_offsets_;

  char*                                     property_buffer_data_{nullptr};
  std::unique_ptr<rhi::IBuffer>             property_buffer_{nullptr};
  std::unordered_map<std::string, uint32_t> property_offsets_;

  friend class Compiler;
};

}  // namespace liger::shader