/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_utils.hpp
 * @date 2024-02-04
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

#include <liger/core/log/default_log.hpp>
#include <liger/render/rhi/rhi_log_channel.hpp>
#include <liger/render/rhi/device.hpp>

#include <vulkan/vulkan.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullability-extension"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop

#define VULKAN_CALL(Call)                                                              \
  {                                                                                    \
    VkResult result = Call;                                                            \
    LIGER_ASSERT(result == VK_SUCCESS, kLogChannelRHI, "Vulkan call error occurred!"); \
  }

namespace liger::rhi {

inline constexpr IDevice::Type GetDeviceTypeFromVulkan(VkPhysicalDeviceType vk_device_type) {
  switch (vk_device_type) {
    case (VK_PHYSICAL_DEVICE_TYPE_OTHER):          { return IDevice::Type::kUndefined; }
    case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU): { return IDevice::Type::kIntegratedGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):   { return IDevice::Type::kDiscreteGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):    { return IDevice::Type::kVirtualGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_CPU):            { return IDevice::Type::kCPU; }

    default:                                       { return IDevice::Type::kUndefined; }
  }
}

inline constexpr uint8_t GetMaxSamplesFromVulkan(const VkPhysicalDeviceProperties& vk_properties) {
  VkSampleCountFlags counts =
      vk_properties.limits.framebufferColorSampleCounts & vk_properties.limits.framebufferDepthSampleCounts;

  uint8_t max_samples = 1;
  if      ((counts & VK_SAMPLE_COUNT_64_BIT) != 0) { max_samples = 64; }
  else if ((counts & VK_SAMPLE_COUNT_32_BIT) != 0) { max_samples = 32; }
  else if ((counts & VK_SAMPLE_COUNT_16_BIT) != 0) { max_samples = 16; }
  else if ((counts & VK_SAMPLE_COUNT_8_BIT)  != 0) { max_samples = 8;  }
  else if ((counts & VK_SAMPLE_COUNT_4_BIT)  != 0) { max_samples = 4;  }
  else if ((counts & VK_SAMPLE_COUNT_2_BIT)  != 0) { max_samples = 2;  }

  return max_samples;
}

inline constexpr VkFormat GetVulkanFormat(Format format) {
  switch (format) {
    /* One-component */
    case (Format::kR32_UINT):            { return VK_FORMAT_R32_UINT; }
    case (Format::kR32_SINT):            { return VK_FORMAT_R32_SINT; }
    case (Format::kR32_SFLOAT):          { return VK_FORMAT_R32_SFLOAT; }

    case (Format::kD16_UNORM):           { return VK_FORMAT_D16_UNORM; }
    case (Format::kD32_SFLOAT):          { return VK_FORMAT_D32_SFLOAT; }

    /* Two-component */
    case (Format::kR32G32_UINT):         { return VK_FORMAT_R32G32_UINT; }
    case (Format::kR32G32_SINT):         { return VK_FORMAT_R32G32_SINT; }
    case (Format::kR32G32_SFLOAT):       { return VK_FORMAT_R32G32_SFLOAT; }

    case (Format::kD24_UNORM_S8_UINT):   { return VK_FORMAT_D24_UNORM_S8_UINT; }

    /* Three-component */
    case (Format::kR8G8B8_UNORM):        { return VK_FORMAT_R8G8B8_UNORM; }
    case (Format::kR8G8B8_SRGB):         { return VK_FORMAT_R8G8B8_SRGB; }

    case (Format::kR16G16B16_SFLOAT):    { return VK_FORMAT_R16G16B16_SFLOAT; }
    case (Format::kR32G32B32_SFLOAT):    { return VK_FORMAT_R32G32B32_SFLOAT; }

    /* Four-component */
    case (Format::kR8G8B8A8_UNORM):      { return VK_FORMAT_R8G8B8A8_UNORM; }
    case (Format::kR8G8B8A8_SRGB):       { return VK_FORMAT_R8G8B8A8_SRGB; }
    case (Format::kB8G8R8A8_SRGB):       { return VK_FORMAT_B8G8R8A8_SRGB; }
    case (Format::kR32G32B32A32_SFLOAT): { return VK_FORMAT_R32G32B32A32_SFLOAT; }

    default:                             { return VK_FORMAT_UNDEFINED; }
  }
}

inline constexpr VkExtent2D GetVulkanExtent2D(Extent2D extent) {
  return VkExtent2D{.width = extent.x, .height = extent.y};
}

inline constexpr VkExtent3D GetVulkanExtent3D(Extent3D extent) {
  return VkExtent3D{.width = extent.x, .height = extent.y, .depth = extent.z};
}

inline constexpr VkBufferUsageFlags GetVulkanBufferUsage(DeviceResourceState states) {
  VkBufferUsageFlags vk_usage = 0;

  if ((states & DeviceResourceState::kTransferSrc) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }

  if ((states & DeviceResourceState::kTransferDst) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  if ((states & DeviceResourceState::kVertexBuffer) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }

  if ((states & DeviceResourceState::kIndexBuffer) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }

  if ((states & DeviceResourceState::kIndirectArgument) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  }

  if ((states & DeviceResourceState::kUniformBuffer) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }

  if ((states & DeviceResourceState::kStorageBuffer) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  return vk_usage;
}

inline constexpr VkImageUsageFlags GetVulkanImageUsage(DeviceResourceState states) {
  VkImageUsageFlags vk_usage = 0;

  if ((states & DeviceResourceState::kTransferSrc) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if ((states & DeviceResourceState::kTransferDst) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  if ((states & DeviceResourceState::kShaderSampled) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if ((states & DeviceResourceState::kColorTarget) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  if ((states & DeviceResourceState::kDepthStencilTarget) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if ((states & DeviceResourceState::kDepthStencilRead) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if ((states & DeviceResourceState::kStorageTexture) != DeviceResourceState::kUndefined) {
    vk_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  return vk_usage;
}

inline constexpr VkImageType GetVulkanImageType(TextureType type) {
  return static_cast<VkImageType>(type);
}

inline constexpr VkImageViewType GetVulkanImageViewType(TextureViewType type) {
  return static_cast<VkImageViewType>(type);
}

inline constexpr VkFilter GetVulkanFilter(Filter filter) {
  return static_cast<VkFilter>(filter);
}

inline constexpr VkSamplerMipmapMode GetVulkanSamplerMipmapMode(Filter mipmap_mode) {
  return static_cast<VkSamplerMipmapMode>(mipmap_mode);
}

inline constexpr VkSamplerAddressMode GetVulkanSamplerAddressMode(SamplerInfo::AddressMode address_mode) {
  return static_cast<VkSamplerAddressMode>(address_mode);
}

inline constexpr VkBorderColor GetVulkanBorderColor(SamplerInfo::BorderColor border_color) {
  return static_cast<VkBorderColor>(border_color);
}

}  // namespace liger::rhi