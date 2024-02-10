/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_compute_pipeline.cpp
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

#include <liger/render/rhi/vulkan/vulkan_compute_pipeline.hpp>

#include <liger/render/rhi/vulkan/vulkan_shader_module.hpp>

namespace liger::rhi {

VulkanComputePipeline::VulkanComputePipeline(VkDevice vk_device) : vk_device_(vk_device) {}

VulkanComputePipeline::~VulkanComputePipeline() {
  if (vk_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(vk_device_, vk_pipeline_, nullptr);
    vk_pipeline_ = VK_NULL_HANDLE;
  }

  if (vk_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(vk_device_, vk_layout_, nullptr);
    vk_layout_ = VK_NULL_HANDLE;
  }
}

bool VulkanComputePipeline::Init(const Info& info) {
  /* Pipeline layout */

  // TODO(tralf-strues): Descriptor set layout after bindless resources are added

  const bool push_constant_present = info.push_constant.size > 0;

  const VkPushConstantRange push_constant_range {
    .stageFlags = GetVulkanShaderStageFlags(info.push_constant.shader_types),
    .offset     = 0,
    .size       = info.push_constant.size
  };

  const VkPipelineLayoutCreateInfo layout_info {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = nullptr,
    .flags                  = 0,
    .setLayoutCount         = 0, // TODO(tralf-strues): Bindless resources!
    .pSetLayouts            = nullptr, // TODO(tralf-strues): Bindless resources!
    .pushConstantRangeCount = push_constant_present ? 1U : 0U,
    .pPushConstantRanges    = push_constant_present ? &push_constant_range : nullptr
  };

  VULKAN_CALL(vkCreatePipelineLayout(vk_device_, &layout_info, nullptr, &vk_layout_));

  /* Pipeline */
  const VkPipelineShaderStageCreateInfo stage_create_info {
    .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext               = nullptr,
    .flags               = 0,
    .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
    .module              = dynamic_cast<VulkanShaderModule*>(info.shader_module)->GetVulkanHandle(),
    .pName               = "main",
    .pSpecializationInfo = nullptr  // TODO(tralf-strues): Add specialization constants to Vulkan RHI
  };

  const VkComputePipelineCreateInfo pipeline_info {
    .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .pNext              = nullptr,
    .flags              = 0,
    .stage              = stage_create_info,
    .layout             = vk_layout_,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex  = 0
  };

  VULKAN_CALL(vkCreateComputePipelines(vk_device_, nullptr, 1, &pipeline_info, nullptr, &vk_pipeline_));

  return true;
}

VkPipeline VulkanComputePipeline::GetVulkanHandle() {
  return vk_pipeline_;
}

}  // namespace liger::rhi