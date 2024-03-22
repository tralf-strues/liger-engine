/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_graphics_pipeline.hpp
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

#pragma once

#include <liger/rhi/graphics_pipeline.hpp>
#include <liger/rhi/vulkan/vulkan_utils.hpp>

namespace liger::rhi {

class VulkanGraphicsPipeline : public IGraphicsPipeline {
 public:
  explicit VulkanGraphicsPipeline(VkDevice vk_device);
  ~VulkanGraphicsPipeline() override;

  bool Init(const Info& info, VkDescriptorSetLayout ds_layout);

  VkPipeline       GetVulkanPipeline() const;
  VkPipelineLayout GetVulkanLayout() const;

 private:
  VkDevice         vk_device_{VK_NULL_HANDLE};
  VkPipelineLayout vk_layout_{VK_NULL_HANDLE};
  VkPipeline       vk_pipeline_{VK_NULL_HANDLE};
};

}  // namespace liger::rhi