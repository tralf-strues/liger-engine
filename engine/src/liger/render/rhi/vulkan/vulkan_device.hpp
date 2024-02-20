/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_device.hpp
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

#include <liger/render/rhi/device.hpp>
#include <liger/render/rhi/vulkan/vulkan_descriptor_manager.hpp>
#include <liger/render/rhi/vulkan/vulkan_queue_set.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullability-extension"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop

namespace liger::rhi {

class VulkanDevice : public IDevice {
 public:
  static constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

  static constexpr const char* kRequiredDeviceExtensions[] = {VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

  VulkanDevice(Info info, uint32_t frames_in_flight, VkInstance instance, VkPhysicalDevice physical_device);
  ~VulkanDevice() override;

  bool Init();

  VulkanQueueSet& GetQueues();

  const Info& GetInfo() const override;
  uint32_t GetFramesInFlight() const override;

  uint32_t AddTextureDescriptor();

  uint32_t BeginFrame(ISwapchain* swapchain) override;
  void EndFrame() override;

  void BeginOffscreenFrame() override;
  void EndOffscreenFrame() override;

  uint32_t CurrentFrame() const override;

  void Execute(RenderGraph& render_graph) override;

  RenderGraphBuilder NewRenderGraphBuilder() override;

  [[nodiscard]] std::unique_ptr<ISwapchain> CreateSwapchain(const ISwapchain::Info& info) override;
  [[nodiscard]] std::unique_ptr<ITexture> CreateTexture(const ITexture::Info& info) override;
  [[nodiscard]] std::unique_ptr<IBuffer> CreateBuffer(const IBuffer::Info& info) override;
  [[nodiscard]] std::unique_ptr<IShaderModule> CreateShaderModule(const IShaderModule::Source& source) override;
  [[nodiscard]] std::unique_ptr<IComputePipeline> CreatePipeline(const IComputePipeline::Info& info) override;
  [[nodiscard]] std::unique_ptr<IGraphicsPipeline> CreatePipeline(const IGraphicsPipeline::Info& info) override;

 private:
  Info     info_;
  uint32_t frames_in_flight_{0};

  VkInstance       instance_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  VkDevice         device_{VK_NULL_HANDLE};
  VmaAllocator     vma_allocator_{nullptr};

  VulkanDescriptorManager descriptor_manager_;
  VulkanQueueSet          queue_set_;
};

}  // namespace liger::rhi