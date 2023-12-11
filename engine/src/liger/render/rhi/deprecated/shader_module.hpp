/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file shader_module.hpp
 * @date 2023-09-26
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

#include <liger/core/core.hpp>
#include <liger/render/rhi/format.hpp>

namespace liger::rhi {

enum class ShaderModuleType : uint32 {
  kVertex,
  kFragment,
  kCompute,
};

enum class ShaderStageBit : uint32 {
  kVertex      = 0x0000'0001,

  /* TODO: other stages */

  kFragment    = 0x0000'0010,
  kCompute     = 0x0000'0020,
};

using ShaderStages = EnumBitMask<ShaderStageBit, uint32>;

enum class ShaderLanguage : uint32 {
  kVulkanGLSL,
  /* kOpenglGLSL */
  /* kHLSL */
};

inline ShaderStageBit GetShaderStageBitFromShaderModuleType(ShaderModuleType type) {
  switch (type) {
    case ShaderModuleType::kVertex:   { return ShaderStageBit::kVertex; }
    case ShaderModuleType::kFragment: { return ShaderStageBit::kFragment; }
    case ShaderModuleType::kCompute:  { return ShaderStageBit::kCompute; }

    default: {
      LIGER_ASSERT(false, LogChannel::kRender, "Invalid ShaderModuleType!");
    }
  }
}

/* Shader Attribute Layout */
struct ShaderAttribute {
  std::string name;
  Format      format;
  uint32      location{0};
};

struct ShaderAttributeLayout {
  std::vector<ShaderAttribute> attribs;
};

}  // namespace liger::rhi