/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ShaderModule.hpp
 * @date 2024-02-03
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

#include <Liger-Engine/Core/EnumBitmask.hpp>

#include <cstdint>
#include <span>

namespace liger::rhi {

class IShaderModule {
 public:
  enum class Type : uint16_t {
    None     = 0,
    Vertex   = Bit(0),
    Fragment = Bit(1),
    Compute  = Bit(2),

    // TODO (tralf-strues): add other shader types
  };

  struct Source {
    /** Shader module type of the source binary. */
    Type type;

    /** Source binary in SPIR-V format. */
    std::span<const uint32_t> source_binary;
  };

  virtual ~IShaderModule() = default;

  Type GetType() const { return type_; }

 protected:
  explicit IShaderModule(Type type) : type_(type) {}

 private:
  Type type_{Type::None};
};

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::IShaderModule::Type);