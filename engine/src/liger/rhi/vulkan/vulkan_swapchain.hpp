/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_swapchain.hpp
 * @date 2024-02-10
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

#include <liger/rhi/swapchain.hpp>
#include <liger/rhi/vulkan/vulkan_texture.hpp>
#include <liger/rhi/vulkan/vulkan_utils.hpp>

namespace liger::rhi {

class VulkanSwapchain : public ISwapchain {
 public:
  VulkanSwapchain(Info info, VulkanDevice& device);
  ~VulkanSwapchain() override;

  bool Init();

  std::vector<ITexture*> GetTextures() override;

  bool Recreate() override;

  VkSwapchainKHR GetVulkanSwapchain();

  std::optional<uint32_t> AcquireNext(VkSemaphore signal_semaphore);

 private:
  struct SurfaceInfo {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
  };

  bool CreateSwapchain();
  SurfaceInfo QuerySurfaceInfo() const;

  VulkanDevice&                               device_;
  VkSwapchainKHR                              swapchain_{VK_NULL_HANDLE};
  VkSurfaceKHR                                surface_{VK_NULL_HANDLE};
  SurfaceInfo                                 surface_info_{};
  std::vector<std::unique_ptr<VulkanTexture>> textures_;
};

}  // namespace liger::rhi