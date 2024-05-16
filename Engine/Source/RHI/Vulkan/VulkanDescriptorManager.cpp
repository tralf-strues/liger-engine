/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanDescriptorManager.cpp
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

#include "VulkanDescriptorManager.hpp"

#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"

namespace liger::rhi {

bool VulkanDescriptorManager::Init(VulkanDevice& device) {
  device_ = device.GetVulkanDevice();

  /* Descriptor set layout */
  const VkDescriptorSetLayoutBinding bindings[]{
    {
      .binding            = kBindingUniformBuffer,
      .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount    = kMaxBindlessResourcesPerType,
      .stageFlags         = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr
    },
    {
      .binding            = kBindingStorageBuffer,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount    = kMaxBindlessResourcesPerType,
      .stageFlags         = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr
    },
    {
      .binding            = kBindingSampledTexture,
      .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount    = kMaxBindlessResourcesPerType,
      .stageFlags         = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr
    },
    {
      .binding            = kBindingStorageTexture,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .descriptorCount    = kMaxBindlessResourcesPerType,
      .stageFlags         = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr
    }
  };

  constexpr auto kBindingsCount = static_cast<uint32_t>(std::size(bindings));
  constexpr auto kBindingFlags =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

  std::array<VkDescriptorBindingFlags, kBindingsCount> binding_flags;
  std::fill(binding_flags.begin(), binding_flags.end(), kBindingFlags);

  const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .pNext         = nullptr,
    .bindingCount  = kBindingsCount,
    .pBindingFlags = binding_flags.data()
  };

  const VkDescriptorSetLayoutCreateInfo layout_info {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext        = &binding_flags_info,
    .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
    .bindingCount = kBindingsCount,
    .pBindings    = bindings
  };

  VULKAN_CALL(vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &layout_));
  device.SetDebugName(layout_, "VulkanDescriptorManager::layout_");

  /* Descriptor pool */
  const VkDescriptorPoolSize pool_sizes[kBindingsCount] {
    {
      .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = kMaxBindlessResourcesPerType
    },
    {
      .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = kMaxBindlessResourcesPerType
    },
    {
      .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = kMaxBindlessResourcesPerType
    },
    {
      .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .descriptorCount = kMaxBindlessResourcesPerType
    }
  };

  const VkDescriptorPoolCreateInfo pool_info {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext         = nullptr,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
    .maxSets       = 1,
    .poolSizeCount = kBindingsCount,
    .pPoolSizes    = pool_sizes
  };

  VULKAN_CALL(vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_));
  device.SetDebugName(pool_, "VulkanDescriptorManager::pool_");

  /* Allocate descriptor set */
  const VkDescriptorSetAllocateInfo allocate_info {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext              = nullptr,
    .descriptorPool     = pool_,
    .descriptorSetCount = 1,
    .pSetLayouts        = &layout_
  };

  VULKAN_CALL(vkAllocateDescriptorSets(device_, &allocate_info, &set_));
  device.SetDebugName(set_, "VulkanDescriptorManager::set_");

  /* Initialize free binding sets */
  auto initialize_set = [](auto& free_bindings) {
    free_bindings.reserve(kMaxBindlessResourcesPerType);
    for (uint32_t binding = 1; binding < kMaxBindlessResourcesPerType; ++binding) {
      free_bindings.insert(binding);
    }
  };

  initialize_set(free_bindings_uniform_buffer_);
  initialize_set(free_bindings_storage_buffer_);
  initialize_set(free_bindings_sampled_texture_);
  initialize_set(free_bindings_storage_texture_);

  /* Create default sampler */
  const VkSamplerCreateInfo sampler_info {
    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext                   = nullptr,
    .flags                   = 0,
    .magFilter               = VK_FILTER_LINEAR,
    .minFilter               = VK_FILTER_LINEAR,
    .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias              = 0,
    .anisotropyEnable        = VK_TRUE,  // FIXME (tralf-strues):
    .maxAnisotropy           = 4.0,      // FIXME (tralf-strues):
    .compareEnable           = VK_FALSE,
    .compareOp               = VK_COMPARE_OP_ALWAYS,
    .minLod                  = 0.0,
    .maxLod                  = VK_LOD_CLAMP_NONE,
    .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };

  VULKAN_CALL(vkCreateSampler(device_, &sampler_info, /*allocator=*/nullptr, &sampler_));
  device.SetDebugName(sampler_, "VulkanDescriptorManager::sampler_");

  return true;
}

void VulkanDescriptorManager::Destroy() {
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }

  if (pool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_, pool_, nullptr);
    pool_ = VK_NULL_HANDLE;
  }

  if (sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(device_, sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
  }

  set_ = VK_NULL_HANDLE;
}

VkDescriptorSetLayout VulkanDescriptorManager::GetLayout() const {
  return layout_;
}

VkDescriptorSet VulkanDescriptorManager::GetSet() const {
  return set_;
}

VulkanDescriptorManager::BufferBindings VulkanDescriptorManager::AddBuffer(VkBuffer            buffer,
                                                                           DeviceResourceState buffer_usage) {
  BufferBindings bindings{};

  const VkDescriptorBufferInfo buffer_info {
    .buffer = buffer,
    .offset = 0,
    .range  = VK_WHOLE_SIZE
  };

  VkWriteDescriptorSet writes[2U];

  uint32_t writes_count = 0;
  if (EnumBitmaskContains(buffer_usage, DeviceResourceState::UniformBuffer)) {
    LIGER_ASSERT(!free_bindings_uniform_buffer_.empty(), kLogChannelRHI, "Max bindless uniform buffers limit reached!");

    uint32_t uniform_binding = *free_bindings_uniform_buffer_.begin();
    free_bindings_uniform_buffer_.extract(free_bindings_uniform_buffer_.begin());
    bindings.uniform = static_cast<BufferDescriptorBinding>(uniform_binding);

    writes[writes_count++] = {
      .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext            = nullptr,
      .dstSet           = set_,
      .dstBinding       = kBindingUniformBuffer,
      .dstArrayElement  = uniform_binding,
      .descriptorCount  = 1,
      .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo       = nullptr,
      .pBufferInfo      = &buffer_info,
      .pTexelBufferView = nullptr
    };
  }

  if (EnumBitmaskContainsAny(buffer_usage, DeviceResourceState::StorageBufferRead |
                                           DeviceResourceState::StorageBufferWrite)) {
    LIGER_ASSERT(!free_bindings_storage_buffer_.empty(), kLogChannelRHI, "Max bindless storage buffers limit reached!");

    uint32_t storage_binding = *free_bindings_storage_buffer_.begin();
    free_bindings_storage_buffer_.extract(free_bindings_storage_buffer_.begin());
    bindings.storage = static_cast<BufferDescriptorBinding>(storage_binding);

    writes[writes_count++] = {
      .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext            = nullptr,
      .dstSet           = set_,
      .dstBinding       = kBindingStorageBuffer,
      .dstArrayElement  = storage_binding,
      .descriptorCount  = 1,
      .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo       = nullptr,
      .pBufferInfo      = &buffer_info,
      .pTexelBufferView = nullptr
    };
  }

  if (writes_count > 0) {
    vkUpdateDescriptorSets(device_, writes_count, writes, 0, nullptr);
  }

  return bindings;
}

void VulkanDescriptorManager::RemoveBuffer(BufferBindings bindings) {
  if (bindings.uniform != BufferDescriptorBinding::Invalid) {
    free_bindings_uniform_buffer_.insert(static_cast<uint32_t>(bindings.uniform));
  }

  if (bindings.storage != BufferDescriptorBinding::Invalid) {
    free_bindings_storage_buffer_.insert(static_cast<uint32_t>(bindings.storage));
  }
}

VulkanDescriptorManager::TextureBindings VulkanDescriptorManager::AddImageView(VkImageView         view,
                                                                               DeviceResourceState texture_usage,
                                                                               VkSampler           sampler) {
  TextureBindings bindings{};

  VkDescriptorImageInfo image_info {
    .sampler     = (sampler != kUseDefaultSampler) ? sampler : sampler_,
    .imageView   = view,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };

  VkWriteDescriptorSet writes[2U];

  uint32_t writes_count = 0;
  if (EnumBitmaskContains(texture_usage, DeviceResourceState::ShaderSampled)) {
    LIGER_ASSERT(!free_bindings_sampled_texture_.empty(), kLogChannelRHI,
                 "Max bindless sampled textures limit reached!");

    uint32_t sampled_binding = *free_bindings_sampled_texture_.begin();
    free_bindings_sampled_texture_.extract(free_bindings_sampled_texture_.begin());
    bindings.sampled = static_cast<TextureDescriptorBinding>(sampled_binding);

    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    writes[writes_count++] = {
      .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext            = nullptr,
      .dstSet           = set_,
      .dstBinding       = kBindingSampledTexture,
      .dstArrayElement  = sampled_binding,
      .descriptorCount  = 1,
      .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo       = &image_info,
      .pBufferInfo      = nullptr,
      .pTexelBufferView = nullptr
    };
  }

  if (EnumBitmaskContainsAny(texture_usage, DeviceResourceState::StorageTextureRead |
                                            DeviceResourceState::StorageTextureWrite)) {
    LIGER_ASSERT(!free_bindings_storage_texture_.empty(), kLogChannelRHI,
                 "Max bindless storage textures limit reached!");

    uint32_t storage_binding = *free_bindings_storage_texture_.begin();
    free_bindings_storage_texture_.extract(free_bindings_storage_texture_.begin());
    bindings.storage = static_cast<TextureDescriptorBinding>(storage_binding);

    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    writes[writes_count++] = {
      .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext            = nullptr,
      .dstSet           = set_,
      .dstBinding       = kBindingStorageTexture,
      .dstArrayElement  = storage_binding,
      .descriptorCount  = 1,
      .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .pImageInfo       = &image_info,
      .pBufferInfo      = nullptr,
      .pTexelBufferView = nullptr
    };
  }

  if (writes_count > 0) {
    vkUpdateDescriptorSets(device_, writes_count, writes, 0, nullptr);
  }

  return bindings;
}

void VulkanDescriptorManager::UpdateSampler(TextureDescriptorBinding sampled_binding, VkImageView view,
                                            VkSampler sampler) {
  const VkDescriptorImageInfo image_info {
    .sampler     = (sampler != kUseDefaultSampler) ? sampler : sampler_,
    .imageView   = view,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };

  const VkWriteDescriptorSet write {
    .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext            = nullptr,
    .dstSet           = set_,
    .dstBinding       = static_cast<uint32_t>(sampled_binding),
    .dstArrayElement  = 0,
    .descriptorCount  = 1,
    .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo       = &image_info,
    .pBufferInfo      = nullptr,
    .pTexelBufferView = nullptr
  };

  vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanDescriptorManager::RemoveImageView(TextureBindings bindings) {
  if (bindings.sampled != TextureDescriptorBinding::Invalid) {
    free_bindings_sampled_texture_.insert(static_cast<uint32_t>(bindings.sampled));
  }

  if (bindings.storage != TextureDescriptorBinding::Invalid) {
    free_bindings_storage_texture_.insert(static_cast<uint32_t>(bindings.storage));
  }
}

}  // namespace liger::rhi