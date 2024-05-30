/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ShaderAlignment.hpp
 * @date 2024-03-03
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

#include <Liger-Engine/Core/Log/Log.hpp>
#include <Liger-Engine/Core/Math/Math.hpp>
#include <Liger-Engine/RHI/DescriptorBinding.hpp>
#include <Liger-Engine/RHI/LogChannel.hpp>

#include <cassert>

namespace glm {
using ivec2 = ::glm::vec<2, int>;
}

#define SHADER_ARRAY_MEMBER(T)  liger::rhi::ArrayMemberAligned<T>
#define SHADER_STRUCT_MEMBER(T) alignas(liger::rhi::StructMemberAlignment<T>::Value()) T

namespace liger::rhi {

template <typename T>
struct ArrayMemberAligned {
  alignas(16) T value;

  ArrayMemberAligned() = default;
  explicit ArrayMemberAligned(const T& value) : value(value) {}

  explicit operator T() const { return value; }
};

template <typename T>
struct StructMemberAlignment {
  static constexpr uint32_t Value() {
    LIGER_ASSERT(false, kLogChannelRHI, "You have to specialize this struct for your type")
    return 0;
  }
};

template <>
struct StructMemberAlignment<int32_t> {
  static constexpr uint32_t Value() { return 4; }
};

template <>
struct StructMemberAlignment<uint32_t> {
  static constexpr uint32_t Value() { return 4; }
};

template <>
struct StructMemberAlignment<rhi::BufferDescriptorBinding> {
  static constexpr uint32_t Value() { return 4; }
};

template <>
struct StructMemberAlignment<rhi::TextureDescriptorBinding> {
  static constexpr uint32_t Value() { return 4; }
};

template <>
struct StructMemberAlignment<float> {
  static constexpr uint32_t Value() { return 4; }
};

template <>
struct StructMemberAlignment<glm::vec2> {
  static constexpr uint32_t Value() { return 8; }
};

template <>
struct StructMemberAlignment<glm::ivec2> {
  static constexpr uint32_t Value() { return 8; }
};

template <>
struct StructMemberAlignment<glm::vec3> {
  static constexpr uint32_t Value() { return 16; }
};

template <>
struct StructMemberAlignment<glm::vec4> {
  static constexpr uint32_t Value() { return 16; }
};

template <>
struct StructMemberAlignment<glm::mat3> {
  static constexpr uint32_t Value() { return 16; }
};

template <>
struct StructMemberAlignment<glm::mat4> {
  static constexpr uint32_t Value() { return 16; }
};

}  // namespace liger::rhi