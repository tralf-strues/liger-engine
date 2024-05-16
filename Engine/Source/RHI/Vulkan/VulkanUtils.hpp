/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanUtils.hpp
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

#include <Liger-Engine/Core/EnumBitmask.hpp>
#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/RHI/Device.hpp>
#include <Liger-Engine/RHI/LogChannel.hpp>

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <vulkan/vk_enum_string_helper.h>

#include <vk_mem_alloc.h>

// MSVC macros
#undef UpdateResource

// X11 has too many common name macros...
#undef Always
#undef Status
#undef None

#define VULKAN_CALL(Call)                                                                           \
  {                                                                                                 \
    VkResult result = Call;                                                                         \
    LIGER_ASSERT(result == VK_SUCCESS, kLogChannelRHI, "Vulkan call error occurred, result = {0}!", \
                 string_VkResult(result));                                                          \
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
    case (VK_PHYSICAL_DEVICE_TYPE_OTHER):          { return IDevice::Type::Undefined; }
    case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU): { return IDevice::Type::IntegratedGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):   { return IDevice::Type::DiscreteGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):    { return IDevice::Type::VirtualGPU; }
    case (VK_PHYSICAL_DEVICE_TYPE_CPU):            { return IDevice::Type::CPU; }

    default:                                       { return IDevice::Type::Undefined; }
  }
}

inline constexpr VkShaderStageFlags GetVulkanShaderStageFlags(IShaderModule::Type mask) {
  VkShaderStageFlags vk_flags = 0;
  if (static_cast<uint16_t>(mask & IShaderModule::Type::Vertex) != 0)   { vk_flags |= VK_SHADER_STAGE_VERTEX_BIT; }
  if (static_cast<uint16_t>(mask & IShaderModule::Type::Fragment) != 0) { vk_flags |= VK_SHADER_STAGE_FRAGMENT_BIT; }
  if (static_cast<uint16_t>(mask & IShaderModule::Type::Compute) != 0)  { vk_flags |= VK_SHADER_STAGE_COMPUTE_BIT; }

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
    case (Format::R32_UINT):            { return VK_FORMAT_R32_UINT; }
    case (Format::R32_SINT):            { return VK_FORMAT_R32_SINT; }
    case (Format::R32_SFLOAT):          { return VK_FORMAT_R32_SFLOAT; }

    case (Format::D16_UNORM):           { return VK_FORMAT_D16_UNORM; }
    case (Format::D32_SFLOAT):          { return VK_FORMAT_D32_SFLOAT; }

    /* Two-component */
    case (Format::R32G32_UINT):         { return VK_FORMAT_R32G32_UINT; }
    case (Format::R32G32_SINT):         { return VK_FORMAT_R32G32_SINT; }
    case (Format::R32G32_SFLOAT):       { return VK_FORMAT_R32G32_SFLOAT; }

    case (Format::D24_UNORM_S8_UINT):   { return VK_FORMAT_D24_UNORM_S8_UINT; }

    /* Three-component */
    case (Format::R8G8B8_UNORM):        { return VK_FORMAT_R8G8B8_UNORM; }
    case (Format::R8G8B8_SRGB):         { return VK_FORMAT_R8G8B8_SRGB; }

    case (Format::R16G16B16_SFLOAT):    { return VK_FORMAT_R16G16B16_SFLOAT; }
    case (Format::R32G32B32_SFLOAT):    { return VK_FORMAT_R32G32B32_SFLOAT; }

    /* Four-component */
    case (Format::R8G8B8A8_UNORM):      { return VK_FORMAT_R8G8B8A8_UNORM; }
    case (Format::R8G8B8A8_SRGB):       { return VK_FORMAT_R8G8B8A8_SRGB; }
    case (Format::B8G8R8A8_SRGB):       { return VK_FORMAT_B8G8R8A8_SRGB; }
    case (Format::R32G32B32A32_SFLOAT): { return VK_FORMAT_R32G32B32A32_SFLOAT; }

    default:                            { return VK_FORMAT_UNDEFINED; }
  }
}

inline constexpr Format GetFormatFromVulkan(VkFormat vk_format) {
  switch (vk_format) {
    /* One-component */
    case (VK_FORMAT_R32_UINT):            { return Format::R32_UINT; }
    case (VK_FORMAT_R32_SINT):            { return Format::R32_SINT; }
    case (VK_FORMAT_R32_SFLOAT):          { return Format::R32_SFLOAT; }

    case (VK_FORMAT_D16_UNORM):           { return Format::D16_UNORM; }
    case (VK_FORMAT_D32_SFLOAT):          { return Format::D32_SFLOAT; }

    /* Two-component */
    case (VK_FORMAT_R32G32_UINT):         { return Format::R32G32_UINT; }
    case (VK_FORMAT_R32G32_SINT):         { return Format::R32G32_SINT; }
    case (VK_FORMAT_R32G32_SFLOAT):       { return Format::R32G32_SFLOAT; }

    case (VK_FORMAT_D24_UNORM_S8_UINT):   { return Format::D24_UNORM_S8_UINT; }

    /* Three-component */
    case (VK_FORMAT_R8G8B8_UNORM):        { return Format::R8G8B8_UNORM; }
    case (VK_FORMAT_R8G8B8_SRGB):         { return Format::R8G8B8_SRGB; }

    case (VK_FORMAT_R16G16B16_SFLOAT):    { return Format::R16G16B16_SFLOAT; }
    case (VK_FORMAT_R32G32B32_SFLOAT):    { return Format::R32G32B32_SFLOAT; }

    /* Four-component */
    case (VK_FORMAT_R8G8B8A8_UNORM):      { return Format::R8G8B8A8_UNORM; }
    case (VK_FORMAT_R8G8B8A8_SRGB):       { return Format::R8G8B8A8_SRGB; }
    case (VK_FORMAT_B8G8R8A8_SRGB):       { return Format::B8G8R8A8_SRGB; }
    case (VK_FORMAT_R32G32B32A32_SFLOAT): { return Format::R32G32B32A32_SFLOAT; }

    default:                              { return Format::Invalid; }
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

  if (EnumBitmaskContains(states, DeviceResourceState::TransferSrc)) {
    vk_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::TransferDst)) {
    vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::VertexBuffer)) {
    vk_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::IndexBuffer)) {
    vk_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::IndirectArgument)) {
    vk_usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::UniformBuffer)) {
    vk_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }

  if (EnumBitmaskContainsAny(states, DeviceResourceState::StorageBufferReadWrite)) {
    vk_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  return vk_usage;
}

inline constexpr VkImageUsageFlags GetVulkanImageUsage(DeviceResourceState states) {
  VkImageUsageFlags vk_usage = 0;

  if (EnumBitmaskContains(states, DeviceResourceState::TransferSrc)) {
    vk_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::TransferDst)) {
    vk_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::ShaderSampled)) {
    vk_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::ColorTarget)) {
    vk_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::DepthStencilTarget)) {
    vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if (EnumBitmaskContains(states, DeviceResourceState::DepthStencilRead)) {
    vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if (EnumBitmaskContainsAny(states, DeviceResourceState::StorageTextureReadWrite)) {
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
    case RasterizationInfo::CullMode::None:         { return VK_CULL_MODE_NONE; }
    case RasterizationInfo::CullMode::FrontOnly:    { return VK_CULL_MODE_FRONT_BIT; }
    case RasterizationInfo::CullMode::BackOnly:     { return VK_CULL_MODE_BACK_BIT; }
    case RasterizationInfo::CullMode::FrontAndBack: { return VK_CULL_MODE_FRONT_AND_BACK; }
    default:                                        { return VK_CULL_MODE_NONE; }
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
  VkAccessFlags2 vk_access = VK_ACCESS_2_NONE;

  if (EnumBitmaskContains(state, DeviceResourceState::TransferSrc)) {
    vk_access |= VK_ACCESS_2_TRANSFER_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::TransferDst)) {
    vk_access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::ShaderSampled)) {
    vk_access |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::ColorTarget)) {
    vk_access |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::DepthStencilTarget)) {
    vk_access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::DepthStencilRead)) {
    vk_access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::StorageTextureRead)) {
    vk_access |= VK_ACCESS_2_SHADER_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::StorageTextureWrite)) {
    vk_access |= VK_ACCESS_2_SHADER_WRITE_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::PresentTexture)) {
    vk_access |= VK_ACCESS_2_NONE;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::VertexBuffer)) {
    vk_access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::IndexBuffer)) {
    vk_access |= VK_ACCESS_2_INDEX_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::IndirectArgument)) {
    vk_access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::UniformBuffer)) {
    vk_access |= VK_ACCESS_2_UNIFORM_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::StorageBufferRead)) {
    vk_access |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
  }

  if (EnumBitmaskContains(state, DeviceResourceState::StorageBufferWrite)) {
    vk_access |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
  }

  return vk_access;
}

inline constexpr VkImageLayout GetVulkanImageLayout(DeviceResourceState state) {
  switch (state) {
    case (DeviceResourceState::Undefined):           { return VK_IMAGE_LAYOUT_UNDEFINED; }
    case (DeviceResourceState::TransferSrc):         { return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; }
    case (DeviceResourceState::TransferDst):         { return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; }
    case (DeviceResourceState::ShaderSampled):       { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
    case (DeviceResourceState::ColorTarget):         { return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; }
    case (DeviceResourceState::DepthStencilTarget):  { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }
    case (DeviceResourceState::DepthStencilRead):    { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; }
    case (DeviceResourceState::StorageTextureRead):  { return VK_IMAGE_LAYOUT_GENERAL; }
    case (DeviceResourceState::StorageTextureWrite): { return VK_IMAGE_LAYOUT_GENERAL; }
    case (DeviceResourceState::PresentTexture):      { return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; }

    default:                                         { return VK_IMAGE_LAYOUT_UNDEFINED; }
  }
}

inline constexpr VkAttachmentLoadOp GetVulkanAttachmentLoadOp(AttachmentLoad load) {
  return static_cast<VkAttachmentLoadOp>(load);
}

inline constexpr VkAttachmentStoreOp GetVulkanAttachmentStoreOp(AttachmentStore store) {
  return static_cast<VkAttachmentStoreOp>(store);
}

inline constexpr VkShaderStageFlags GetVulkanAllComputePipelineShaderStages() {
  return VK_SHADER_STAGE_COMPUTE_BIT;
}

inline constexpr VkShaderStageFlags GetVulkanAllGraphicsPipelineShaderStages() {
  return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // TODO (tralf-strues): add the rest stages
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