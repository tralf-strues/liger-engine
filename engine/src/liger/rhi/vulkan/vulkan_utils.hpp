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

#include <liger/core/enum_bitmask.hpp>
#include <liger/core/enum_reflection.hpp>
#include <liger/core/log/default_log.hpp>
#include <liger/rhi/device.hpp>
#include <liger/rhi/rhi_log_channel.hpp>

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <vulkan/vk_enum_string_helper.h>

#include <vk_mem_alloc.h>

#define VULKAN_CALL(Call)                                                              \
  {                                                                                    \
    VkResult result = Call;                                                            \
    LIGER_ASSERT(result == VK_SUCCESS, kLogChannelRHI, "Vulkan call error occurred!"); \
  }

template <>
struct fmt::formatter<VkResult> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const VkResult& vk_result, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", liger::EnumToString(vk_result));
  }
};

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

inline constexpr VkShaderStageFlags GetVulkanShaderStageFlags(IShaderModule::Type mask) {
  VkShaderStageFlags vk_flags = 0;
  if (static_cast<uint16_t>(mask & IShaderModule::Type::kVertex) != 0) { vk_flags |= VK_SHADER_STAGE_VERTEX_BIT; }
  if (static_cast<uint16_t>(mask & IShaderModule::Type::kFragment) != 0) { vk_flags |= VK_SHADER_STAGE_FRAGMENT_BIT; }
  if (static_cast<uint16_t>(mask & IShaderModule::Type::kCompute) != 0) { vk_flags |= VK_SHADER_STAGE_COMPUTE_BIT; }

  return vk_flags;
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

inline constexpr Format GetFormatFromVulkan(VkFormat vk_format) {
  switch (vk_format) {
    /* One-component */
    case (VK_FORMAT_R32_UINT):            { return Format::kR32_UINT; }
    case (VK_FORMAT_R32_SINT):            { return Format::kR32_SINT; }
    case (VK_FORMAT_R32_SFLOAT):          { return Format::kR32_SFLOAT; }

    case (VK_FORMAT_D16_UNORM):           { return Format::kD16_UNORM; }
    case (VK_FORMAT_D32_SFLOAT):          { return Format::kD32_SFLOAT; }

    /* Two-component */
    case (VK_FORMAT_R32G32_UINT):         { return Format::kR32G32_UINT; }
    case (VK_FORMAT_R32G32_SINT):         { return Format::kR32G32_SINT; }
    case (VK_FORMAT_R32G32_SFLOAT):       { return Format::kR32G32_SFLOAT; }

    case (VK_FORMAT_D24_UNORM_S8_UINT):   { return Format::kD24_UNORM_S8_UINT; }

    /* Three-component */
    case (VK_FORMAT_R8G8B8_UNORM):        { return Format::kR8G8B8_UNORM; }
    case (VK_FORMAT_R8G8B8_SRGB):         { return Format::kR8G8B8_SRGB; }

    case (VK_FORMAT_R16G16B16_SFLOAT):    { return Format::kR16G16B16_SFLOAT; }
    case (VK_FORMAT_R32G32B32_SFLOAT):    { return Format::kR32G32B32_SFLOAT; }

    /* Four-component */
    case (VK_FORMAT_R8G8B8A8_UNORM):      { return Format::kR8G8B8A8_UNORM; }
    case (VK_FORMAT_R8G8B8A8_SRGB):       { return Format::kR8G8B8A8_SRGB; }
    case (VK_FORMAT_B8G8R8A8_SRGB):       { return Format::kB8G8R8A8_SRGB; }
    case (VK_FORMAT_R32G32B32A32_SFLOAT): { return Format::kR32G32B32A32_SFLOAT; }

    default:                              { return Format::kInvalid; }
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

inline constexpr VkPrimitiveTopology GetVulkanPrimitiveTopology(InputAssemblyInfo::Topology topology) {
  return static_cast<VkPrimitiveTopology>(topology);
}

inline constexpr VkPolygonMode GetVulkanPolygonMode(RasterizationInfo::PolygonMode polygon_mode) {
  return static_cast<VkPolygonMode>(polygon_mode);
}

inline constexpr VkCullModeFlags GetVulkanCullMode(RasterizationInfo::CullMode cull_mode) {
  switch (cull_mode) {
    case RasterizationInfo::CullMode::kNone:         { return VK_CULL_MODE_NONE; }
    case RasterizationInfo::CullMode::kFrontOnly:    { return VK_CULL_MODE_FRONT_BIT; }
    case RasterizationInfo::CullMode::kBackOnly:     { return VK_CULL_MODE_BACK_BIT; }
    case RasterizationInfo::CullMode::kFrontAndBack: { return VK_CULL_MODE_FRONT_AND_BACK; }
    default:                                         { return VK_CULL_MODE_NONE; }
  }
}

inline constexpr VkFrontFace GetVulkanFrontFace(RasterizationInfo::FrontFace front_face) {
  return static_cast<VkFrontFace>(front_face);
}

inline constexpr VkCompareOp GetVulkanCompareOp(DepthStencilTestInfo::CompareOperation operation) {
  return static_cast<VkCompareOp>(operation);
}

inline constexpr VkBlendFactor GetVulkanBlendFactor(ColorBlendInfo::Factor factor) {
  return static_cast<VkBlendFactor>(factor);
}

inline constexpr VkBlendOp GetVulkanBlendOp(ColorBlendInfo::Operation operation) {
  return static_cast<VkBlendOp>(operation);
}

inline constexpr VkAccessFlags2 GetVulkanAccessFlags(DeviceResourceState state) {
  VkAccessFlags2 vk_access = 0;

  if ((state & DeviceResourceState::kTransferSrc) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_TRANSFER_READ_BIT;
  }

  if ((state & DeviceResourceState::kTransferDst) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }

  if ((state & DeviceResourceState::kShaderSampled) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
  }

  if ((state & DeviceResourceState::kColorTarget) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  }

  if ((state & DeviceResourceState::kDepthStencilTarget) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }

  if ((state & DeviceResourceState::kDepthStencilRead) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }

  if ((state & DeviceResourceState::kStorageTexture) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  }

  if ((state & DeviceResourceState::kPresentTexture) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_NONE;
  }

  if ((state & DeviceResourceState::kVertexBuffer) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
  }

  if ((state & DeviceResourceState::kIndexBuffer) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_INDEX_READ_BIT;
  }

  if ((state & DeviceResourceState::kIndirectArgument) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
  }

  if ((state & DeviceResourceState::kUniformBuffer) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_UNIFORM_READ_BIT;
  }

  if ((state & DeviceResourceState::kStorageBuffer) != DeviceResourceState::kUndefined) {
    vk_access |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
  }

  return vk_access;
}

inline constexpr VkImageLayout GetVulkanImageLayout(DeviceResourceState state) {
  switch (state) {
    case (DeviceResourceState::kUndefined):          { return VK_IMAGE_LAYOUT_UNDEFINED; }
    case (DeviceResourceState::kTransferSrc):        { return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; }
    case (DeviceResourceState::kTransferDst):        { return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; }
    case (DeviceResourceState::kShaderSampled):      { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
    case (DeviceResourceState::kColorTarget):        { return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; }
    case (DeviceResourceState::kDepthStencilTarget): { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }
    case (DeviceResourceState::kDepthStencilRead):   { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; }
    case (DeviceResourceState::kStorageTexture):     { return VK_IMAGE_LAYOUT_GENERAL; }
    case (DeviceResourceState::kPresentTexture):     { return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; }

    default:                                         { return VK_IMAGE_LAYOUT_UNDEFINED; }
  }
}

inline constexpr VkAttachmentLoadOp GetVulkanAttachmentLoadOp(AttachmentLoad load) {
  return static_cast<VkAttachmentLoadOp>(load);
}

inline constexpr VkAttachmentStoreOp GetVulkanAttachmentStoreOp(AttachmentStore store) {
  return static_cast<VkAttachmentStoreOp>(store);
}

template <typename VulkanHandleT>
inline constexpr VkObjectType GetVulkanObjectType();

#define LIGER_GET_VULKAN_OBJECT_TYPE(VulkanHandleT, value)             \
  template <>                                                          \
  inline constexpr VkObjectType GetVulkanObjectType<VulkanHandleT>() { \
    return value;                                                      \
  }

LIGER_GET_VULKAN_OBJECT_TYPE(VkInstance, VK_OBJECT_TYPE_INSTANCE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkPhysicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkDevice, VK_OBJECT_TYPE_DEVICE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkQueue, VK_OBJECT_TYPE_QUEUE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)
LIGER_GET_VULKAN_OBJECT_TYPE(VkFence, VK_OBJECT_TYPE_FENCE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY)
LIGER_GET_VULKAN_OBJECT_TYPE(VkBuffer, VK_OBJECT_TYPE_BUFFER)
LIGER_GET_VULKAN_OBJECT_TYPE(VkImage, VK_OBJECT_TYPE_IMAGE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkEvent, VK_OBJECT_TYPE_EVENT)
LIGER_GET_VULKAN_OBJECT_TYPE(VkQueryPool, VK_OBJECT_TYPE_QUERY_POOL)
LIGER_GET_VULKAN_OBJECT_TYPE(VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW)
LIGER_GET_VULKAN_OBJECT_TYPE(VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW)
LIGER_GET_VULKAN_OBJECT_TYPE(VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT)
LIGER_GET_VULKAN_OBJECT_TYPE(VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS)
LIGER_GET_VULKAN_OBJECT_TYPE(VkPipeline, VK_OBJECT_TYPE_PIPELINE)
LIGER_GET_VULKAN_OBJECT_TYPE(VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)
LIGER_GET_VULKAN_OBJECT_TYPE(VkSampler, VK_OBJECT_TYPE_SAMPLER)
LIGER_GET_VULKAN_OBJECT_TYPE(VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL)
LIGER_GET_VULKAN_OBJECT_TYPE(VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET)
LIGER_GET_VULKAN_OBJECT_TYPE(VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER)
LIGER_GET_VULKAN_OBJECT_TYPE(VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL)
LIGER_GET_VULKAN_OBJECT_TYPE(VkSurfaceKHR, VK_OBJECT_TYPE_SURFACE_KHR)
LIGER_GET_VULKAN_OBJECT_TYPE(VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR)

#undef LIGER_GET_VULKAN_OBJECT_TYPE

}  // namespace liger::rhi