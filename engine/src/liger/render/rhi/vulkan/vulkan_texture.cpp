/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_texture.cpp
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

#include <liger/render/rhi/vulkan/vulkan_texture.hpp>

namespace liger::rhi {

VulkanTexture::VulkanTexture(Info info, VkDevice vk_device, VmaAllocator vma_allocator,
                             VulkanDescriptorManager& descriptor_manager)
    : ITexture(std::move(info)),
      vk_device_(vk_device),
      vma_allocator_(vma_allocator),
      descriptor_manager_(descriptor_manager) {}

VulkanTexture::VulkanTexture(Info info, VkDevice vk_device, VkImage vk_image,
                             VulkanDescriptorManager& descriptor_manager)
    : ITexture(std::move(info)),
      owning_(false),
      vk_device_(vk_device),
      vk_image_(vk_image),
      descriptor_manager_(descriptor_manager) {}

VulkanTexture::~VulkanTexture() {
  for (auto& view : views_) {
    if (view.vk_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_device_, view.vk_view, nullptr);
    }

    if (view.vk_custom_sampler != VK_NULL_HANDLE) {
      vkDestroySampler(vk_device_, view.vk_custom_sampler, nullptr);
    }

    descriptor_manager_.RemoveImageView(view.bindings);

    view.vk_view           = VK_NULL_HANDLE;
    view.vk_custom_sampler = VK_NULL_HANDLE;
  }

  if (vk_image_ != VK_NULL_HANDLE && owning_) {
    vmaDestroyImage(vma_allocator_, vk_image_, vma_allocation_);
    vk_image_ = VK_NULL_HANDLE;
    vma_allocation_ = VK_NULL_HANDLE;
  }
}

bool VulkanTexture::Init() {
  if (owning_) {
    /* Create image */
    const uint8_t sample_count = GetInfo().samples;
    if (sample_count == 0 || (sample_count & (sample_count - 1)) != 0) {
      LIGER_LOG_ERROR(kLogChannelRHI,
                      "Texture sample count must be greater than zero and be a power of two, but it is set to {}!",
                      sample_count);
      return false;
    }

    const VkImageCreateInfo image_info {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext     = nullptr,
      .flags     = static_cast<VkImageCreateFlags>(GetInfo().cube_compatible ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0),
      .imageType = GetVulkanImageType(GetInfo().type),
      .format    = GetVulkanFormat(GetInfo().format),
      .extent    = GetVulkanExtent3D(GetInfo().extent),
      .mipLevels = GetInfo().mip_levels,
      .arrayLayers           = GetLayerCount(),
      .samples               = static_cast<VkSampleCountFlagBits>(sample_count),
      .tiling                = VK_IMAGE_TILING_OPTIMAL, // TODO(tralf-strues): CPU visible textures
      .usage                 = GetVulkanImageUsage(GetInfo().usage),
      .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices   = nullptr,
      .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage  = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // TODO(tralf-strues): CPU visible textures
    alloc_info.flags |= 0; // TODO(tralf-strues): CPU visible textures
    
    VULKAN_CALL(vmaCreateImage(vma_allocator_, &image_info, &alloc_info, &vk_image_, &vma_allocation_, nullptr));
  }

  /* Create default image view */
  TextureViewType view_type;
  if (GetInfo().type == TextureType::kTexture3D) { view_type = TextureViewType::k3D; }

  if (GetInfo().type == TextureType::kTexture1D) {
    view_type = (GetLayerCount() == 1) ? TextureViewType::k1D : TextureViewType::kArray1D;
  }

  if (GetInfo().type == TextureType::kTexture2D) {
    view_type = (GetLayerCount() == 1) ? TextureViewType::k2D : TextureViewType::kArray2D;
  }

  const TextureViewInfo default_view_info{
    .type        = view_type,
    .first_mip   = 0,
    .mip_count   = GetInfo().mip_levels,
    .first_layer = 0,
    .layer_count = GetLayerCount()
  };

  CreateView(default_view_info);

  return true;
}

uint32_t VulkanTexture::CreateView(const TextureViewInfo& info) {
  VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_NONE;
  if (IsDepthStencilFormat(GetInfo().format)) {
    aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  } else if (IsDepthContainingFormat(GetInfo().format)) {
    aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
  } else {
    aspect_mask |= VK_IMAGE_ASPECT_COLOR_BIT;
  }

  const VkImageViewCreateInfo view_info {
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext    = nullptr,
    .flags    = 0,
    .image    = vk_image_,
    .viewType = GetVulkanImageViewType(info.type),
    .format   = GetVulkanFormat(GetInfo().format),

    .components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY
    },

    .subresourceRange = {
      .aspectMask     = aspect_mask,
      .baseMipLevel   = info.first_mip,
      .levelCount     = info.mip_count,
      .baseArrayLayer = info.first_layer,
      .layerCount     = info.layer_count,
    }
  };

  VkImageView vk_view{VK_NULL_HANDLE};
  VULKAN_CALL(vkCreateImageView(vk_device_, &view_info, nullptr, &vk_view));

  auto const bindings = descriptor_manager_.AddImageView(vk_view, GetInfo().usage);

  const auto view_idx = static_cast<uint32_t>(views_.size());
  views_.emplace_back(SampledView{.vk_view = vk_view, .vk_custom_sampler = VK_NULL_HANDLE, .bindings = bindings});

  return view_idx;
}

TextureDescriptorBinding VulkanTexture::GetSampledDescriptorBinding(uint32_t view) const {
  LIGER_ASSERT(view < views_.size(), kLogChannelRHI, "Trying to access invalid view index!");
  return views_[view].bindings.sampled;
}

TextureDescriptorBinding VulkanTexture::GetStorageDescriptorBinding(uint32_t view) const {
  LIGER_ASSERT(view < views_.size(), kLogChannelRHI, "Trying to access invalid view index!");
  return views_[view].bindings.storage;
}

bool VulkanTexture::SetSampler(const SamplerInfo& info, uint32_t view_idx) {
  LIGER_ASSERT(view_idx < views_.size(), kLogChannelRHI, "Trying to access invalid view index!");

  auto& view = views_[view_idx];

  if (!EnumBitmaskContains(GetInfo().usage, DeviceResourceState::kShaderSampled) ||
      view.bindings.sampled == TextureDescriptorBinding::kInvalid) {
    return false;
  }

  const VkSamplerCreateInfo sampler_info{
    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext                   = nullptr,
    .flags                   = 0,
    .magFilter               = GetVulkanFilter(info.mag_filter),
    .minFilter               = GetVulkanFilter(info.min_filter),
    .mipmapMode              = GetVulkanSamplerMipmapMode(info.mipmap_mode),
    .addressModeU            = GetVulkanSamplerAddressMode(info.address_mode_u),
    .addressModeV            = GetVulkanSamplerAddressMode(info.address_mode_v),
    .addressModeW            = GetVulkanSamplerAddressMode(info.address_mode_w),
    .mipLodBias              = info.lod_bias,
    .anisotropyEnable        = static_cast<VkBool32>(info.anisotropy_enabled),
    .maxAnisotropy           = info.max_anisotropy,
    .compareEnable           = VK_FALSE,  // TODO(tralf-strues): What is this? (Mainly used for shadow maps)
    .compareOp               = VK_COMPARE_OP_ALWAYS,
    .minLod                  = info.min_lod,
    .maxLod                  = info.max_lod,
    .borderColor             = GetVulkanBorderColor(info.border_color),
    .unnormalizedCoordinates = VK_FALSE
  };

  VkSampler vk_sampler{VK_NULL_HANDLE};
  VULKAN_CALL(vkCreateSampler(vk_device_, &sampler_info, nullptr, &vk_sampler));

  view.vk_custom_sampler = vk_sampler;

  descriptor_manager_.UpdateSampler(view.bindings.sampled, view.vk_view, vk_sampler);

  return true;
}

VkImageView VulkanTexture::GetVulkanView(uint32_t view_idx) const {
  LIGER_ASSERT(view_idx < views_.size(), kLogChannelRHI, "Trying to access invalid view index!");

  return views_[view_idx].vk_view;
}

uint32_t VulkanTexture::GetLayerCount() const {
  return (GetInfo().type != TextureType::kTexture3D) ? GetInfo().extent.z : 1;
}

}  // namespace liger::rhi