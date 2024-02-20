/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_device.cpp
 * @date 2024-02-06
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

#include <liger/render/rhi/vulkan/vulkan_device.hpp>

#include <liger/render/rhi/vulkan/vulkan_buffer.hpp>
#include <liger/render/rhi/vulkan/vulkan_compute_pipeline.hpp>
#include <liger/render/rhi/vulkan/vulkan_graphics_pipeline.hpp>
#include <liger/render/rhi/vulkan/vulkan_shader_module.hpp>
#include <liger/render/rhi/vulkan/vulkan_swapchain.hpp>
#include <liger/render/rhi/vulkan/vulkan_texture.hpp>
#include <liger/render/rhi/vulkan/vulkan_utils.hpp>

#define VMA_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullability-extension"
#pragma GCC diagnostic ignored "-Wnullability-completeness"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop

namespace liger::rhi {

VulkanDevice::VulkanDevice(Info info, uint32_t frames_in_flight, VkInstance instance, VkPhysicalDevice physical_device)
    : info_(std::move(info)),
      frames_in_flight_(frames_in_flight),
      instance_(instance),
      physical_device_(physical_device) {}

VulkanDevice::~VulkanDevice() {
  descriptor_manager_.Destroy();

  if (vma_allocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(vma_allocator_);
    vma_allocator_ = VK_NULL_HANDLE;
  }

  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
}

bool VulkanDevice::Init() {
  auto queue_create_infos = queue_set_.FillQueueCreateInfos(physical_device_);

  VkPhysicalDeviceFeatures2 device_features2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  device_features2.features.samplerAnisotropy = VK_TRUE;

  std::vector<const char*> extensions{std::begin(kRequiredDeviceExtensions), std::end(kRequiredDeviceExtensions)};

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature {
    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
    .dynamicRendering = VK_TRUE
  };

  VkPhysicalDeviceDescriptorIndexingFeatures indexing_features {
    .sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    .pNext                                         = &dynamic_rendering_feature,
    .descriptorBindingPartiallyBound               = VK_TRUE,
    .runtimeDescriptorArray                        = VK_TRUE,
    .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
    .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
    .descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE,
    .descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE
  };

  device_features2.pNext = &indexing_features;

  VkDeviceCreateInfo create_info {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &device_features2,
    .flags = 0,
    .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
    .pQueueCreateInfos = queue_create_infos.data(),
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data(),
    .pEnabledFeatures = nullptr,
  };

  VULKAN_CALL(vkCreateDevice(physical_device_, &create_info, nullptr, &device_));

  volkLoadDevice(device_);

  queue_set_.InitQueues(device_);

  VmaVulkanFunctions vma_vulkan_functions = {};
  vma_vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vma_vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice   = physical_device_;
  allocator_info.device           = device_;
  allocator_info.instance         = instance_;
  allocator_info.pVulkanFunctions = &vma_vulkan_functions;
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
  VULKAN_CALL(vmaCreateAllocator(&allocator_info, &vma_allocator_));

  return descriptor_manager_.Init(device_);
}

VulkanQueueSet& VulkanDevice::GetQueues() {
  return queue_set_;
}

const VulkanDevice::Info& VulkanDevice::GetInfo() const { return info_; }
uint32_t VulkanDevice::GetFramesInFlight() const { return frames_in_flight_; }

uint32_t VulkanDevice::BeginFrame(ISwapchain* /*swapchain*/) {
  return 0;  // TODO(tralf-strues): Implement!
}

void VulkanDevice::EndFrame() {
  // TODO(tralf-strues): Implement!
}

void VulkanDevice::BeginOffscreenFrame() {
  // TODO(tralf-strues): Implement!
}

void VulkanDevice::EndOffscreenFrame() {
  // TODO(tralf-strues): Implement!
}

uint32_t VulkanDevice::CurrentFrame() const {
  return 0;  // TODO(tralf-strues): Implement!
}

void VulkanDevice::Execute(RenderGraph& /*render_graph*/) {
  // TODO(tralf-strues): Implement!
}

RenderGraphBuilder VulkanDevice::NewRenderGraphBuilder() {
  // TODO(tralf-strues): Implement!
  return RenderGraphBuilder{nullptr};
}

std::unique_ptr<ISwapchain> VulkanDevice::CreateSwapchain(const ISwapchain::Info& info) {
  auto swapchain = std::make_unique<VulkanSwapchain>(info, instance_, device_, descriptor_manager_);

  if (!swapchain->Init(physical_device_)) {
    return nullptr;
  }

  return swapchain;
}

std::unique_ptr<ITexture> VulkanDevice::CreateTexture(const ITexture::Info& info) {
  auto texture = std::make_unique<VulkanTexture>(info, device_, vma_allocator_, descriptor_manager_);

  if (!texture->Init()) {
    return nullptr;
  }

  return texture;
}

std::unique_ptr<IBuffer> VulkanDevice::CreateBuffer(const IBuffer::Info& info) {
  auto buffer = std::make_unique<VulkanBuffer>(info, vma_allocator_, descriptor_manager_);

  if (!buffer->Init()) {
    return nullptr;
  }

  return buffer;
}

std::unique_ptr<IShaderModule> VulkanDevice::CreateShaderModule(const IShaderModule::Source& source) {
  auto shader_module = std::make_unique<VulkanShaderModule>(device_, source.type);

  if (!shader_module->Init(source)) {
    return nullptr;
  }

  return shader_module;
}

std::unique_ptr<IComputePipeline> VulkanDevice::CreatePipeline(const IComputePipeline::Info& info) {
  auto compute_pipeline = std::make_unique<VulkanComputePipeline>(device_);

  if (!compute_pipeline->Init(info)) {
    return nullptr;
  }

  return compute_pipeline;
}

std::unique_ptr<IGraphicsPipeline> VulkanDevice::CreatePipeline(const IGraphicsPipeline::Info& info) {
  auto graphics_pipeline = std::make_unique<VulkanGraphicsPipeline>(device_);

  if (!graphics_pipeline->Init(info)) {
    return nullptr;
  }

  return graphics_pipeline;
}

}  // namespace liger::rhi