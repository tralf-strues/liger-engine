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

#include <liger/core/enum_bitmask.hpp>
#include <liger/core/log/default_log.hpp>
#include <liger/render/rhi/device_resource.hpp>

namespace liger::rhi {

enum class ShaderModuleType : uint8_t {
  kVertex,
  kFragment,
  kCompute,
};

enum class ShaderStages : uint32_t {
  kNone        = 0,
  kVertex      = 0x0000'0001,

  /* TODO: other stages */

  kFragment    = 0x0000'0010,
  kCompute     = 0x0000'0020,
};

enum class ShaderLanguage : uint8_t {
  kVulkanGLSL,
  /* kOpenglGLSL */
  /* kHLSL */
};

inline constexpr ShaderStages GetShaderStageFromShaderModuleType(ShaderModuleType type) {
  switch (type) {
    case ShaderModuleType::kVertex:   { return ShaderStages::kVertex; }
    case ShaderModuleType::kFragment: { return ShaderStages::kFragment; }
    case ShaderModuleType::kCompute:  { return ShaderStages::kCompute; }

    default: {
      LIGER_ASSERT(false, "RHI", "Invalid ShaderModuleType!");
    }
  }
}

struct ShaderModuleBinary {
  ShaderModuleType type;
  uint32_t         size_bytes;
  const uint32_t*  source_binary;
};

class ShaderModule final : DeviceResource {
 public:
  explicit ShaderModule(DeviceResource::InternalHandle internal = nullptr);
  ~ShaderModule() override = default;
};

ShaderModule::ShaderModule(DeviceResource::InternalHandle internal) : DeviceResource(internal) {}

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::ShaderStages);
