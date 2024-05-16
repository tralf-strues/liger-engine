/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanDevice.cpp
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

#include "VulkanDevice.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderGraph.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanTexture.hpp"
#include "VulkanUtils.hpp"

#define VMA_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop

namespace liger::rhi {

VulkanDevice::VulkanDevice(Info info, uint32_t frames_in_flight, VkInstance instance, VkPhysicalDevice physical_device)
    : info_(std::move(info)),
      frames_in_flight_(frames_in_flight),
      instance_(instance),
      physical_device_(physical_device),
      transfer_engine_(*this) {}

VulkanDevice::~VulkanDevice() {
  descriptor_manager_.Destroy();

  render_graph_semaphore_.Destroy();

  for (auto& frame_sync : frame_sync_) {
    if (frame_sync.fence_render_finished != VK_NULL_HANDLE) {
      vkDestroyFence(device_, frame_sync.fence_render_finished, nullptr);
      frame_sync.fence_render_finished = VK_NULL_HANDLE;
    }

    if (frame_sync.semaphore_render_finished != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, frame_sync.semaphore_render_finished, nullptr);
      frame_sync.semaphore_render_finished = VK_NULL_HANDLE;
    }

    if (frame_sync.semaphore_swapchain_acquire != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, frame_sync.semaphore_swapchain_acquire, nullptr);
      frame_sync.semaphore_swapchain_acquire = VK_NULL_HANDLE;
    }
  }

  if (vma_allocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(vma_allocator_);
    vma_allocator_ = VK_NULL_HANDLE;
  }

  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
}

bool VulkanDevice::Init(bool debug_enable) {
  debug_enabled_ = debug_enable;

  auto queue_create_infos = queue_set_.FillQueueCreateInfos(physical_device_);

  VkPhysicalDeviceFeatures2 device_features2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  device_features2.features.samplerAnisotropy         = VK_TRUE;
  device_features2.features.shaderInt64               = VK_TRUE;
  device_features2.features.multiDrawIndirect         = VK_TRUE;
  device_features2.features.drawIndirectFirstInstance = VK_TRUE;

  VkPhysicalDeviceVulkan12Features device_features12 {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  device_features12.timelineSemaphore                             = VK_TRUE;
  device_features12.pNext                                         = &device_features2;
  device_features12.descriptorBindingPartiallyBound               = VK_TRUE;
  device_features12.runtimeDescriptorArray                        = VK_TRUE;
  device_features12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
  device_features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
  device_features12.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
  device_features12.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
  device_features12.shaderUniformBufferArrayNonUniformIndexing    = VK_TRUE;
  device_features12.shaderStorageBufferArrayNonUniformIndexing    = VK_TRUE;

  std::vector<const char*> extensions{std::begin(kRequiredDeviceExtensions), std::end(kRequiredDeviceExtensions)};

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  VkPhysicalDeviceSynchronization2FeaturesKHR sync2_features {
    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
    .pNext            = nullptr,
    .synchronization2 = VK_TRUE
  };

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature {
    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
    .pNext            = &sync2_features,
    .dynamicRendering = VK_TRUE
  };

  device_features2.pNext = &dynamic_rendering_feature;

  VkDeviceCreateInfo create_info {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &device_features12,
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

  queue_set_.InitQueues(*this);

  VmaVulkanFunctions vma_vulkan_functions = {};
  vma_vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vma_vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice   = physical_device_;
  allocator_info.device           = device_;
  allocator_info.instance         = instance_;
  allocator_info.pVulkanFunctions = &vma_vulkan_functions;
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
  VULKAN_CALL(vmaCreateAllocator(&allocator_info, &vma_allocator_));

  render_graph_semaphore_.Init(device_, kMaxRenderGraphsPerFrame);
  SetDebugName(render_graph_semaphore_.Get(), "VulkanDevice::render_graph_semaphore_");

  CreateFrameSync();

  constexpr uint64_t kTransferSize = 128U * 1024U * 1024U;
  transfer_engine_.Init(*queue_set_.GetTransferQueue(), *queue_set_.GetQueueFamilyIndices().transfer, kTransferSize);

  return descriptor_manager_.Init(*this);
}

VkInstance                VulkanDevice::GetInstance()             { return instance_; }
VkPhysicalDevice          VulkanDevice::GetPhysicalDevice()       { return physical_device_; }
VkDevice                  VulkanDevice::GetVulkanDevice()         { return device_; }
VulkanQueueSet&           VulkanDevice::GetQueues()               { return queue_set_; }
VmaAllocator              VulkanDevice::GetAllocator()            { return vma_allocator_; }
VulkanDescriptorManager&  VulkanDevice::GetDescriptorManager()    { return descriptor_manager_; }
bool                      VulkanDevice::GetDebugEnabled() const   { return debug_enabled_; }
const VulkanDevice::Info& VulkanDevice::GetInfo() const           { return info_; }
uint32_t                  VulkanDevice::GetFramesInFlight() const { return frames_in_flight_; }

void VulkanDevice::WaitIdle() {
  VULKAN_CALL(vkDeviceWaitIdle(device_));
}

std::optional<uint32_t> VulkanDevice::BeginFrame(ISwapchain& swapchain) {
  current_swapchain_ = static_cast<VulkanSwapchain*>(&swapchain);
  auto& frame_sync = frame_sync_[CurrentFrame()];

  VULKAN_CALL(vkWaitForFences(device_, 1, &frame_sync.fence_render_finished, VK_TRUE, UINT64_MAX));
  VULKAN_CALL(vkResetFences(device_, 1, &frame_sync.fence_render_finished));

  auto next_texture_idx = current_swapchain_->AcquireNext(frame_sync.semaphore_swapchain_acquire);
  if (!next_texture_idx) {
    current_swapchain_ = nullptr;
    IncrementFrame();
    WaitIdle();
    return std::nullopt;
  }

  current_swapchain_image_idx_ = next_texture_idx.value();
  current_graph_idx_ = 0;

  return next_texture_idx;
}

bool VulkanDevice::EndFrame() {
  auto& frame_sync  = frame_sync_[CurrentFrame()];
  auto  empty_frame = (current_graph_idx_ == 0);

  if (!empty_frame) {
    const VkSemaphoreSubmitInfo last_graph_wait {
      .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .pNext       = nullptr,
      .semaphore   = render_graph_semaphore_.Get(),
      .value       = CalculateRenderGraphSemaphoreValue(current_graph_idx_),
      .stageMask   = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
      .deviceIndex = 0
    };

    const VkSemaphoreSubmitInfo render_finished_signal {
      .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .pNext       = nullptr,
      .semaphore   = frame_sync.semaphore_render_finished,
      .value       = 0,
      .stageMask   = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
      .deviceIndex = 0
    };

    const VkSubmitInfo2 submit {
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext                    = nullptr,
      .flags                    = 0,
      .waitSemaphoreInfoCount   = 1,
      .pWaitSemaphoreInfos      = &last_graph_wait,
      .commandBufferInfoCount   = 0,
      .pCommandBufferInfos      = nullptr,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos    = &render_finished_signal
    };

    // FIXME (tralf-strues): fence_render_finished must always be signaled, but currently under condition
    VULKAN_CALL(vkQueueSubmit2(queue_set_.GetMainQueue(), 1, &submit, frame_sync.fence_render_finished));
  }

  const VkSwapchainKHR vk_swapchain = current_swapchain_->GetVulkanSwapchain();

  const VkPresentInfoKHR present_info {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext              = nullptr,
    .waitSemaphoreCount = empty_frame ? 0U : 1U,
    .pWaitSemaphores    = empty_frame ? nullptr : &frame_sync.semaphore_render_finished,
    .swapchainCount     = 1,
    .pSwapchains        = &vk_swapchain,
    .pImageIndices      = &current_swapchain_image_idx_,
    .pResults           = nullptr
  };

  const auto result = vkQueuePresentKHR(queue_set_.GetMainQueue(), &present_info);
  bool       valid  = true;

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    valid = false;
    WaitIdle();
  }

  current_swapchain_ = nullptr;
  IncrementFrame();

  transfer_engine_.SubmitAndWait();

  return valid;
}

void VulkanDevice::BeginOffscreenFrame() {
  // TODO(tralf-strues): Implement!
}

void VulkanDevice::EndOffscreenFrame() {
  // TODO(tralf-strues): Implement!
}

uint32_t VulkanDevice::CurrentFrame() const {
  return current_frame_idx_;
}

uint64_t VulkanDevice::CurrentAbsoluteFrame() const {
  return current_absolute_frame_;
}

void VulkanDevice::ExecuteConsecutive(RenderGraph& render_graph, Context& context) {
  LIGER_ASSERT(current_graph_idx_ + 1 < kMaxRenderGraphsPerFrame, kLogChannelRHI,
               "Trying to execute too many render graphs per frame, the limit is kMaxRenderGraphsPerFrame={}",
               kMaxRenderGraphsPerFrame);

  auto& frame_sync          = frame_sync_[CurrentFrame()];
  auto& vulkan_render_graph = static_cast<VulkanRenderGraph&>(render_graph);

  auto first_graph    = (current_graph_idx_ == 0);
  auto wait_value     = first_graph ? 0 : CalculateRenderGraphSemaphoreValue(current_graph_idx_);
  auto wait_semaphore = first_graph ? frame_sync.semaphore_swapchain_acquire : render_graph_semaphore_.Get();

  ++current_graph_idx_;

  auto signal_value = CalculateRenderGraphSemaphoreValue(current_graph_idx_);

  vulkan_render_graph.Execute(context, wait_semaphore, wait_value, render_graph_semaphore_.Get(), signal_value);
}

void VulkanDevice::RequestDedicatedTransfer(DedicatedTransferRequest&& transfer) {
  transfer_engine_.Request(std::move(transfer));
}

RenderGraphBuilder VulkanDevice::NewRenderGraphBuilder() {
  return RenderGraphBuilder(std::make_unique<VulkanRenderGraph>());
}

std::unique_ptr<ISwapchain> VulkanDevice::CreateSwapchain(const ISwapchain::Info& info) {
  auto swapchain = std::make_unique<VulkanSwapchain>(info, *this);

  if (!swapchain->Init()) {
    return nullptr;
  }

  return swapchain;
}

std::unique_ptr<ITexture> VulkanDevice::CreateTexture(const ITexture::Info& info) {
  auto texture = std::make_unique<VulkanTexture>(info, *this);

  if (!texture->Init()) {
    return nullptr;
  }

  return texture;
}

std::unique_ptr<IBuffer> VulkanDevice::CreateBuffer(const IBuffer::Info& info) {
  auto buffer = std::make_unique<VulkanBuffer>(info, *this);

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

std::unique_ptr<IPipeline> VulkanDevice::CreatePipeline(const IPipeline::GraphicsInfo& info) {
  auto pipeline = std::make_unique<VulkanPipeline>(*this);

  if (!pipeline->Init(info)) {
    return nullptr;
  }

  return pipeline;
}

std::unique_ptr<IPipeline> VulkanDevice::CreatePipeline(const IPipeline::ComputeInfo& info) {
  auto pipeline = std::make_unique<VulkanPipeline>(*this);

  if (!pipeline->Init(info)) {
    return nullptr;
  }

  return pipeline;
}

void VulkanDevice::CreateFrameSync() {
  frame_sync_.resize(GetFramesInFlight());

  for (uint32_t frame_idx = 0; frame_idx < GetFramesInFlight(); ++frame_idx) {
    auto& frame_sync = frame_sync_[frame_idx];

    constexpr VkFenceCreateInfo kFenceInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VULKAN_CALL(vkCreateFence(device_, &kFenceInfo, nullptr, &frame_sync.fence_render_finished));

    SetDebugName(frame_sync.fence_render_finished, "VulkanDevice::frame_sync_[{0}].fence_render_finished", frame_idx);

    constexpr VkSemaphoreCreateInfo kSemaphoreInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
    };

    VULKAN_CALL(vkCreateSemaphore(device_, &kSemaphoreInfo, nullptr, &frame_sync.semaphore_render_finished));
    VULKAN_CALL(vkCreateSemaphore(device_, &kSemaphoreInfo, nullptr, &frame_sync.semaphore_swapchain_acquire));

    SetDebugName(frame_sync.semaphore_render_finished, "VulkanDevice::frame_sync_[{0}].semaphore_render_finished",
                 frame_idx);
    SetDebugName(frame_sync.semaphore_render_finished, "VulkanDevice::frame_sync_[{0}].semaphore_render_finished",
                 frame_idx);
  }
}

void VulkanDevice::IncrementFrame() {
  current_frame_idx_ = (current_frame_idx_ + 1) % frames_in_flight_;
  ++current_absolute_frame_;
}

uint64_t VulkanDevice::CalculateRenderGraphSemaphoreValue(uint64_t render_graph_idx) const {
  return CurrentAbsoluteFrame() * (kMaxRenderGraphsPerFrame + 1) + render_graph_idx + 1;
}

}  // namespace liger::rhi