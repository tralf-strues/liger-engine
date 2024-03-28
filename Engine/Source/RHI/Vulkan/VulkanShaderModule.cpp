/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanShaderModule.cpp
 * @date 2024-02-10
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

#include "VulkanShaderModule.hpp"

namespace liger::rhi {

VulkanShaderModule::VulkanShaderModule(VkDevice vk_device, Type type) : IShaderModule(type), vk_device_(vk_device) {}

VulkanShaderModule::~VulkanShaderModule() {
  if (vk_shader_module_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(vk_device_, vk_shader_module_, nullptr);
    vk_shader_module_ = VK_NULL_HANDLE;
  }
}

bool VulkanShaderModule::Init(const IShaderModule::Source& source) {
  const VkShaderModuleCreateInfo create_info {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = source.source_binary.size_bytes(),
    .pCode = source.source_binary.data(),
  };

  VULKAN_CALL(vkCreateShaderModule(vk_device_, &create_info, nullptr, &vk_shader_module_));

  return true;
}

VkShaderModule VulkanShaderModule::GetVulkanHandle()  const {
  return vk_shader_module_;
}

}  // namespace liger::rhi