/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Instance.cpp
 * @date 2024-02-11
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

#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/RHI/LogChannel.hpp>

#include "Vulkan/VulkanInstance.hpp"

namespace liger::rhi {

std::unique_ptr<IInstance> IInstance::Create(GraphicsAPI api, ValidationLevel validation) {
  switch (api) {
    case GraphicsAPI::Vulkan: {
      auto instance = std::make_unique<VulkanInstance>();
      return instance->Init(validation) ? std::move(instance) : nullptr;
    }

    default: {
      LIGER_LOG_ERROR(kLogChannelRHI, "Graphics API \"{0}\" is not yet implemented!", EnumToString(api));
      return nullptr;
    }
  }
}

}  // namespace liger::rhi