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

#define VK_NO_PROTOTYPES
#include <volk.h>
// #ifdef __APPLE__
// #define VK_USE_PLATFORM_MACOS_MVK
// #define VK_USE_PLATFORM_METAL_EXT
// #define VK_ENABLE_BETA_EXTENSIONS
// #endif
// #include <vulkan/vulkan.h>

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

  static constexpr uint32_t kBindingUniformBuffer  = 0;
  static constexpr uint32_t kBindingStorageBuffer  = 1;
  static constexpr uint32_t kBindingSampledTexture = 2;
  static constexpr uint32_t kBindingStorageTexture = 3;

  static constexpr uint32_t kMaxBindlessResourcesPerType = 1024;

  VulkanDevice(Info info, uint32_t frames_in_flight, VkInstance instance, VkPhysicalDevice physical_device);
  ~VulkanDevice() override;

  bool Init();

  const Info& GetInfo() const override;
  uint32_t GetFramesInFlight() const override;

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
  struct QueueFamilyIndices {
    uint32_t main;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
  };

  bool FindQueueFamilies();
  bool SetupBindless();

  Info     info_;
  uint32_t frames_in_flight_{0};

  VkInstance       instance_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  VkDevice         device_{VK_NULL_HANDLE};

  VkDescriptorPool      bindless_descriptor_pool_{VK_NULL_HANDLE};
  VkDescriptorSetLayout bindless_descriptor_set_layout_{VK_NULL_HANDLE};
  VkDescriptorSet       bindless_descriptor_set_{VK_NULL_HANDLE};

  QueueFamilyIndices queue_family_indices_;
  VkQueue            main_queue_{VK_NULL_HANDLE};
  VkQueue            compute_queue_{VK_NULL_HANDLE};
  VkQueue            transfer_queue_{VK_NULL_HANDLE};

  VmaAllocator vma_allocator_{nullptr};
};

}  // namespace liger::rhi