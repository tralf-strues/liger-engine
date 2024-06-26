/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanTexture.hpp
 * @date 2024-02-08
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

#include <Liger-Engine/RHI/Texture.hpp>

#include "VulkanDescriptorManager.hpp"
#include "VulkanUtils.hpp"

namespace liger::rhi {

class VulkanDevice;

class VulkanTexture : public ITexture {
 public:
  VulkanTexture(Info info, VulkanDevice& device);
  VulkanTexture(Info info, VulkanDevice& device, VkImage image);
  ~VulkanTexture() override;

  bool Init();

  uint32_t CreateView(const TextureViewInfo& info) override;
  bool ViewCreated(uint32_t view) const override;
  const TextureViewInfo& GetViewInfo(uint32_t view) const override;

  TextureDescriptorBinding GetSampledDescriptorBinding(uint32_t view) const override;
  TextureDescriptorBinding GetStorageDescriptorBinding(uint32_t view) const override;
  bool SetSampler(const SamplerInfo& sampler_info, uint32_t view = kTextureDefaultViewIdx) override;

  VkImage GetVulkanImage() const;
  VkImageView GetVulkanView(uint32_t view) const;

 private:
  struct SampledView {
    VkImageView                              view              {VK_NULL_HANDLE};
    VkSampler                                custom_sampler    {VK_NULL_HANDLE};
    VulkanDescriptorManager::TextureBindings bindings          {};
    TextureViewInfo                          info              {};
  };

  uint32_t GetLayerCount() const;

  VulkanDevice&            device_;
  std::vector<SampledView> views_;
  bool                     owning_     {true};
  VkImage                  image_      {VK_NULL_HANDLE};
  VmaAllocation            allocation_ {VK_NULL_HANDLE};
};

}  // namespace liger::rhi