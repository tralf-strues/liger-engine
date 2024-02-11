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
  if (bindless_descriptor_set_layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_, bindless_descriptor_set_layout_, nullptr);
    bindless_descriptor_set_layout_ = VK_NULL_HANDLE;
  }

  if (bindless_descriptor_pool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_, bindless_descriptor_pool_, nullptr);
    bindless_descriptor_pool_ = VK_NULL_HANDLE;
  }

  bindless_descriptor_set_ = VK_NULL_HANDLE;

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
  if (!FindQueueFamilies()) {
    return false;
  }

  constexpr float kDefaultQueuePriority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_infos[3];

  auto add_queue_create_info = [&](size_t index, uint32_t family_index) {
    queue_create_infos[index] = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = 0,
      .queueFamilyIndex = family_index,
      .queueCount       = 1,
      .pQueuePriorities = &kDefaultQueuePriority
    };
  };

  size_t queue_count = 0;
  add_queue_create_info(queue_count++, queue_family_indices_.main);
  if (queue_family_indices_.compute.has_value()) {
    add_queue_create_info(queue_count++, *queue_family_indices_.compute);
  }
  if (queue_family_indices_.transfer.has_value()) {
    add_queue_create_info(queue_count++, *queue_family_indices_.transfer);
  }

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
    .queueCreateInfoCount = static_cast<uint32_t>(queue_count),
    .pQueueCreateInfos = queue_create_infos,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data(),
    .pEnabledFeatures = nullptr,
  };

  VULKAN_CALL(vkCreateDevice(physical_device_, &create_info, nullptr, &device_));

  volkLoadDevice(device_);

  vkGetDeviceQueue(device_, queue_family_indices_.main, /*queueIndex=*/0, &main_queue_);

  if (queue_family_indices_.compute.has_value()) {
    vkGetDeviceQueue(device_, *queue_family_indices_.compute, /*queueIndex=*/0, &compute_queue_);
  }

  if (queue_family_indices_.transfer.has_value()) {
    vkGetDeviceQueue(device_, *queue_family_indices_.transfer, /*queueIndex=*/0, &transfer_queue_);
  }

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

  return SetupBindless();
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
  auto swapchain = std::make_unique<VulkanSwapchain>(info, instance_, device_);

  if (!swapchain->Init(physical_device_)) {
    return nullptr;
  }

  return swapchain;
}

std::unique_ptr<ITexture> VulkanDevice::CreateTexture(const ITexture::Info& info) {
  auto texture = std::make_unique<VulkanTexture>(info, device_, vma_allocator_);

  if (!texture->Init()) {
    return nullptr;
  }

  return texture;
}

std::unique_ptr<IBuffer> VulkanDevice::CreateBuffer(const IBuffer::Info& info) {
  auto buffer = std::make_unique<VulkanBuffer>(info, vma_allocator_);

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

bool VulkanDevice::FindQueueFamilies() {
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

  /* Main queue */
  bool main_queue_found = false;
  for (indices.main = 0; indices.main < queue_family_count; ++indices.main) {
    const auto& properties = queue_families[indices.main];

    if (properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) {
      main_queue_found = true;
      break;
    }
  }

  if (!main_queue_found) {
    LIGER_LOG_ERROR(kLogChannelRHI, "Failed to find a main vulkan queue that supports graphics, compute and transfer!");
    return false;
  }

  /* Compute queue */
  bool compute_queue_found = false;
  for (indices.compute = 0; indices.compute < queue_family_count; ++(*indices.compute)) {
    const auto& properties = queue_families[*indices.compute];

    if ((properties.queueFlags & VK_QUEUE_COMPUTE_BIT) && indices.compute != indices.main) {
      compute_queue_found = true;
      break;
    }
  }

  if (!compute_queue_found) {
    LIGER_LOG_INFO(kLogChannelRHI, "No async compute vulkan queue is found");
    indices.compute = std::nullopt;
  } else {
    LIGER_LOG_INFO(kLogChannelRHI, "Async compute vulkan queue is found!");
  }

  /* Transfer queue */
  bool transfer_queue_found = false;
  for (indices.transfer = 0; indices.transfer < queue_family_count; ++(*indices.transfer)) {
    const auto& properties = queue_families[*indices.transfer];

    if (properties.queueFlags == VK_QUEUE_TRANSFER_BIT) {
      transfer_queue_found = true;
      break;
    }
  }

  if (!transfer_queue_found) {
    LIGER_LOG_INFO(kLogChannelRHI, "No dedicated vulkan queue for transfer is found");
    indices.transfer = std::nullopt;
  } else {
    LIGER_LOG_INFO(kLogChannelRHI, "Dedicated vulkan queue for transfer is found!");
  }

  queue_family_indices_ = indices;

  return true;
}

bool VulkanDevice::SetupBindless() {
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

  VULKAN_CALL(vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &bindless_descriptor_set_layout_));

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

  VULKAN_CALL(vkCreateDescriptorPool(device_, &pool_info, nullptr, &bindless_descriptor_pool_));

  /* Allocate descriptor set */
  const VkDescriptorSetAllocateInfo allocate_info {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = bindless_descriptor_pool_,
    .descriptorSetCount = 1,
    .pSetLayouts = &bindless_descriptor_set_layout_,
  };

  VULKAN_CALL(vkAllocateDescriptorSets(device_, &allocate_info, &bindless_descriptor_set_));

  return true;
}

}  // namespace liger::rhi