/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Declaration.hpp
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

#include <Liger-Engine/RHI/Pipeline.hpp>

namespace liger::shader {

struct Declaration {
  using Scope = rhi::IShaderModule::Type;

  struct Member {
    enum class Type : uint16_t {
      Invalid,

      UniformBuffer,
      StorageBuffer,

      Sampler2D,
      Sampler2DArray,
      StorageTexture,

      Bool,
      Int32,
      UInt32,
      UInt64,
      Float32,

      U32Vec2,
      U32Vec3,
      U32Vec4,

      F32Vec2,
      F32Vec3,
      F32Vec4,

      F32Mat3,
      F32Mat4,

      VertexIndex,
      InstanceIndex
    };

    enum class BufferLayout : uint16_t {
      Std140,
      Std430
    };

    enum class BufferAccess : uint16_t {
      ReadOnly,
      WriteOnly,
      ReadWrite
    };

    enum class Modifier : uint16_t {
      Property,
      StageIO,
      CompileConstant,
      PushConstant
    };

    std::string  name;
    Type         type{Type::Invalid};

    BufferLayout buffer_layout{BufferLayout::Std140};
    BufferAccess buffer_access{BufferAccess::ReadOnly};
    std::string  buffer_contents;

    Modifier     modifier{Modifier::Property};
  };

  struct CodeSnippet {
    enum class InsertPolicy : uint16_t {
      Global,
      Local
    };

    std::string  name;
    std::string  code;
    InsertPolicy insert{InsertPolicy::Global};
  };

  Scope                                             scope{Scope::None};
  std::string                                       data_block;
  std::vector<Member>                               input;
  std::vector<Member>                               output;
  std::vector<CodeSnippet>                          code_snippets;
  std::string                                       code;

  std::vector<std::string>                          includes;
  std::vector<std::string>                          interfaces;

  std::vector<Declaration>                          declarations;

  std::optional<rhi::InputAssemblyInfo::Topology>   vertex_topology;
  std::optional<rhi::RasterizationInfo>             rasterization;
  std::optional<rhi::DepthStencilTestInfo>          depth_stencil_test;
  std::optional<rhi::ColorBlendInfo>                color_blend;
  std::optional<rhi::AttachmentInfo>                attachments;

  std::optional<std::array<uint32_t, 3U>>           thread_group_size;
};

inline constexpr bool IsTextureType(Declaration::Member::Type type) {
  return type == Declaration::Member::Type::Sampler2D || type == Declaration::Member::Type::Sampler2DArray ||
         type == Declaration::Member::Type::StorageTexture;
}

inline constexpr bool IsBufferType(Declaration::Member::Type type) {
  return type == Declaration::Member::Type::UniformBuffer || type == Declaration::Member::Type::StorageBuffer;
}

inline constexpr bool IsResourceType(Declaration::Member::Type type) {
  return IsTextureType(type) || IsBufferType(type);
}

}  // namespace liger::shader