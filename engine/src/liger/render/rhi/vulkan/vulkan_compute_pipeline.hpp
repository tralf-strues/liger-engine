/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_compute_pipeline.hpp
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

#pragma once

#include <liger/render/rhi/compute_pipeline.hpp>
#include <liger/render/rhi/vulkan/vulkan_utils.hpp>

namespace liger::rhi {

class VulkanComputePipeline : public IComputePipeline {
 public:
  explicit VulkanComputePipeline(VkDevice vk_device);
  ~VulkanComputePipeline() override;

  bool Init(const Info& info);

  VkPipeline GetVulkanHandle();

 private:
  VkDevice         vk_device_{VK_NULL_HANDLE};
  VkPipelineLayout vk_layout_{VK_NULL_HANDLE};
  VkPipeline       vk_pipeline_{VK_NULL_HANDLE};
};

}  // namespace liger::rhi