/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanDescriptorManager.hpp
 * @date 2024-02-12
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

#include <Liger-Engine/RHI/DescriptorBinding.hpp>
#include <Liger-Engine/RHI/DeviceResourceState.hpp>

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <unordered_set>

namespace liger::rhi {

class VulkanDescriptorManager {
 public:
  static constexpr uint32_t kBindingUniformBuffer  = 0;
  static constexpr uint32_t kBindingStorageBuffer  = 1;
  static constexpr uint32_t kBindingSampledTexture = 2;
  static constexpr uint32_t kBindingStorageTexture = 3;

  static constexpr uint32_t kMaxBindlessResourcesPerType = 1024;

  static constexpr VkSampler kUseDefaultSampler = VK_NULL_HANDLE;

  struct BufferBindings {
    BufferDescriptorBinding uniform{BufferDescriptorBinding::Invalid};
    BufferDescriptorBinding storage{BufferDescriptorBinding::Invalid};
  };

  struct TextureBindings {
    TextureDescriptorBinding sampled{TextureDescriptorBinding::Invalid};
    TextureDescriptorBinding storage{TextureDescriptorBinding::Invalid};
  };

  bool Init(VkDevice device);
  void Destroy();

  VkDescriptorSetLayout GetLayout() const;

  [[nodiscard]] BufferBindings AddBuffer(VkBuffer buffer, DeviceResourceState buffer_usage);

  void RemoveBuffer(BufferBindings bindings);

  [[nodiscard]] TextureBindings AddImageView(VkImageView view, DeviceResourceState texture_usage,
                                             VkSampler sampler = kUseDefaultSampler);

  void UpdateSampler(TextureDescriptorBinding sampled_binding, VkImageView view,
                     VkSampler sampler = kUseDefaultSampler);

  void RemoveImageView(TextureBindings bindings);

 private:
  VkDevice              device_  {VK_NULL_HANDLE};
  VkDescriptorPool      pool_    {VK_NULL_HANDLE};
  VkDescriptorSetLayout layout_  {VK_NULL_HANDLE};
  VkDescriptorSet       set_     {VK_NULL_HANDLE};
  VkSampler             sampler_ {VK_NULL_HANDLE};

  std::unordered_set<uint32_t> free_bindings_uniform_buffer_;
  std::unordered_set<uint32_t> free_bindings_storage_buffer_;
  std::unordered_set<uint32_t> free_bindings_sampled_texture_;
  std::unordered_set<uint32_t> free_bindings_storage_texture_;
};

}  // namespace liger::rhi