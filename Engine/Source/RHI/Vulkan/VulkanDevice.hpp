/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanDevice.hpp
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

#pragma once

#include <Liger-Engine/RHI/Device.hpp>

#include "VulkanDescriptorManager.hpp"
#include "VulkanQueueSet.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanTimelineSemaphore.hpp"
#include "VulkanTransferEngine.hpp"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>
#undef VMA_STATIC_VULKAN_FUNCTIONS
#undef VMA_DYNAMIC_VULKAN_FUNCTIONS

namespace liger::rhi {

class VulkanDevice : public IDevice {
 public:
  static constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

  static constexpr const char* kRequiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  static constexpr uint64_t kMaxRenderGraphsPerFrame = 1024;

  VulkanDevice(Info info, uint32_t frames_in_flight, VkInstance instance, VkPhysicalDevice physical_device);
  ~VulkanDevice() override;

  bool Init(bool debug_enabled);

  VkInstance GetInstance();
  VkPhysicalDevice GetPhysicalDevice();
  VkDevice GetVulkanDevice();
  VulkanQueueSet& GetQueues();
  VmaAllocator GetAllocator();
  VulkanDescriptorManager& GetDescriptorManager();

  bool GetDebugEnabled() const;
  const Info& GetInfo() const override;
  uint32_t GetFramesInFlight() const override;

  void WaitIdle() override;

  template <typename VulkanHandleT, typename... FormatArgs>
  inline void SetDebugName(VulkanHandleT handle, std::string_view fmt, FormatArgs&&... args) const;

  std::optional<uint32_t> BeginFrame(ISwapchain& swapchain) override;
  bool EndFrame() override;

  void BeginOffscreenFrame() override;
  void EndOffscreenFrame() override;

  uint32_t CurrentFrame() const override;
  uint32_t NextFrame() const;
  uint64_t CurrentAbsoluteFrame() const override;

  void ExecuteConsecutive(RenderGraph& render_graph, Context& context) override;

  void RequestDedicatedTransfer(DedicatedTransferRequest&& transfer) override;

  RenderGraphBuilder NewRenderGraphBuilder(Context& context) override;

  [[nodiscard]] std::unique_ptr<ISwapchain> CreateSwapchain(const ISwapchain::Info& info) override;
  [[nodiscard]] std::unique_ptr<ITexture> CreateTexture(const ITexture::Info& info) override;
  [[nodiscard]] std::unique_ptr<IBuffer> CreateBuffer(const IBuffer::Info& info) override;
  [[nodiscard]] std::unique_ptr<IShaderModule> CreateShaderModule(const IShaderModule::Source& source) override;
  [[nodiscard]] std::unique_ptr<IPipeline> CreatePipeline(const IPipeline::ComputeInfo& info) override;
  [[nodiscard]] std::unique_ptr<IPipeline> CreatePipeline(const IPipeline::GraphicsInfo& info) override;

 private:
  struct FrameSynchronization {
    VkFence     fence_render_finished{VK_NULL_HANDLE};
    VkSemaphore semaphore_render_finished{VK_NULL_HANDLE};
    VkSemaphore semaphore_swapchain_acquire{VK_NULL_HANDLE};
  };

  void CreateFrameSync();
  void IncrementFrame();
  uint64_t CalculateRenderGraphSemaphoreValue(uint64_t render_graph_idx) const;

  Info     info_;
  bool     debug_enabled_{false};
  uint32_t frames_in_flight_{0};
  uint32_t current_frame_idx_{0};
  uint64_t current_absolute_frame_{0};

  VkInstance       instance_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  VkDevice         device_{VK_NULL_HANDLE};
  VmaAllocator     vma_allocator_{nullptr};

  VulkanDescriptorManager descriptor_manager_;
  VulkanQueueSet          queue_set_;
  VulkanTransferEngine    transfer_engine_;

  std::vector<FrameSynchronization> frame_sync_;
  VulkanSwapchain*                  current_swapchain_{nullptr};
  uint32_t                          current_swapchain_image_idx_{0};

  VulkanTimelineSemaphore render_graph_semaphore_;
  uint64_t                current_graph_idx_{0};
};

template <typename VulkanHandleT, typename... FormatArgs>
void VulkanDevice::SetDebugName(VulkanHandleT handle, std::string_view fmt,
                                FormatArgs&&... args) const {
  if (!debug_enabled_) {
    return;
  }

  auto name = fmt::format(fmt::runtime(fmt), std::forward<FormatArgs>(args)...);

  const VkDebugUtilsObjectNameInfoEXT name_info {
    .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .pNext        = nullptr,
    .objectType   = GetVulkanObjectType<VulkanHandleT>(),
    .objectHandle = reinterpret_cast<uint64_t>(handle),
    .pObjectName  = name.c_str()
  };

  VULKAN_CALL(vkSetDebugUtilsObjectNameEXT(device_, &name_info));
}

}  // namespace liger::rhi