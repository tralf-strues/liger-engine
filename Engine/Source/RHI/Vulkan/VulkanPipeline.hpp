/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanPipeline.hpp
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

#include <Liger-Engine/RHI/Pipeline.hpp>

#include "VulkanUtils.hpp"

namespace liger::rhi {

class VulkanDevice;

class VulkanPipeline : public IPipeline {
 public:
  explicit VulkanPipeline(VulkanDevice& device);
  ~VulkanPipeline() override;

  bool Init(const GraphicsInfo& info);
  bool Init(const ComputeInfo& info);

  VkPipeline          GetVulkanPipeline() const;
  VkPipelineLayout    GetVulkanLayout() const;
  VkPipelineBindPoint GetVulkanBindPoint() const;

 private:
  VulkanDevice&       device_;
  VkPipelineLayout    layout_{VK_NULL_HANDLE};
  VkPipeline          pipeline_{VK_NULL_HANDLE};
  VkPipelineBindPoint bind_point_ = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

}  // namespace liger::rhi