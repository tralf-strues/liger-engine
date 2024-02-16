/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_swapchain.cpp
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

#define VK_NO_PROTOTYPES
#include <volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <liger/render/rhi/vulkan/vulkan_swapchain.hpp>

namespace liger::rhi {

VkSurfaceFormatKHR ChooseSwapchainFormat(std::span<const VkSurfaceFormatKHR> formats) {
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }

  return formats[0];
}

VkPresentModeKHR ChooseSwapchainPresentMode(std::span<const VkPresentModeKHR> present_modes, bool vsync) {
  for (const auto& mode : present_modes) {
    if (!vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      return mode;
    }

    if (vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;  // Guaranteed to be always available
}

VulkanSwapchain::VulkanSwapchain(Info info, VkInstance vk_instance, VkDevice vk_device,
                                 VulkanDescriptorManager& descriptor_manager)
    : ISwapchain(std::move(info)),
      vk_instance_(vk_instance),
      vk_device_(vk_device),
      descriptor_manager_(descriptor_manager) {}

VulkanSwapchain::~VulkanSwapchain() {
  textures_.clear();

  if (vk_swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);
    vk_swapchain_ = VK_NULL_HANDLE;
  }

  if (vk_surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(vk_instance_, vk_surface_, nullptr);
    vk_surface_ = VK_NULL_HANDLE;
  }
}

bool VulkanSwapchain::Init(VkPhysicalDevice vk_physical_device) {
  VULKAN_CALL(glfwCreateWindowSurface(vk_instance_, GetInfo().window->GetGLFWwindow(), nullptr, &vk_surface_));
  surface_info_ = QuerySurfaceInfo(vk_physical_device);

  return CreateSwapchain();
}

std::vector<ITexture*> VulkanSwapchain::GetTextures() {
  std::vector<ITexture*> textures;
  textures.reserve(textures_.size());

  for (auto& owning_texture : textures_) {
    textures.emplace_back(owning_texture.get());
  }

  return textures;
}

bool VulkanSwapchain::Recreate() {
  textures_.clear();

  if (vk_swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);
    vk_swapchain_ = VK_NULL_HANDLE;
  }

  return CreateSwapchain();
}

bool VulkanSwapchain::CreateSwapchain() {
  Window& window = *GetInfo().window;

  auto format          = ChooseSwapchainFormat(surface_info_.formats);
  auto present_mode    = ChooseSwapchainPresentMode(surface_info_.present_modes, GetInfo().vsync);
  auto extent          = VkExtent2D{window.GetFramebufferWidth(), window.GetFramebufferHeight()};
  auto min_image_count = std::max(static_cast<uint32_t>(GetInfo().min_size), surface_info_.capabilities.minImageCount);

  const VkSwapchainCreateInfoKHR swapchain_info {
    .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                 = nullptr,
    .flags                 = 0,
    .surface               = vk_surface_,
    .minImageCount         = min_image_count,
    .imageFormat           = format.format,
    .imageColorSpace       = format.colorSpace,
    .imageExtent           = extent,
    .imageArrayLayers      = 1,
    .imageUsage            = GetVulkanImageUsage(GetInfo().usage),
    .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices   = nullptr,
    .preTransform          = surface_info_.capabilities.currentTransform,
    .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode           = present_mode,
    .clipped               = VK_TRUE,  // Pixels that are obscured by another window are clipped
    .oldSwapchain          = VK_NULL_HANDLE
  };

  VULKAN_CALL(vkCreateSwapchainKHR(vk_device_, &swapchain_info, nullptr, &vk_swapchain_));

  /* Retrieve images */
  uint32_t image_count = 0;
  std::vector<VkImage> vk_images;

  VULKAN_CALL(vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &image_count, nullptr));
  vk_images.resize(image_count);
  VULKAN_CALL(vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, nullptr, vk_images.data()));

  /* Creating textures */
  textures_.reserve(image_count);

  const ITexture::Info texture_info {
    .format          = GetFormatFromVulkan(format.format),
    .type            = TextureType::kTexture2D,
    .usage           = GetInfo().usage,
    .cube_compatible = false,
    .extent          = {.x = extent.width, .y = extent.height, .z = 1},
    .mip_levels      = 1,
    .samples         = 1,
    .name            = ""
  };

  for (auto vk_image : vk_images) {
    auto texture = std::make_unique<VulkanTexture>(texture_info, vk_device_, vk_image, descriptor_manager_);
    if (!texture->Init()) {
      return false;
    }

    textures_.emplace_back(std::move(texture));
  }

  return true;
}

VulkanSwapchain::SurfaceInfo VulkanSwapchain::QuerySurfaceInfo(VkPhysicalDevice vk_physical_device) const {
  SurfaceInfo info;

  /* Surface capabilities */
  VULKAN_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface_, &info.capabilities));

  /* Surface formats */
  uint32_t format_count = 0;
  VULKAN_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface_, &format_count, nullptr));

  if (format_count != 0) {
    info.formats.resize(format_count);
    VULKAN_CALL(
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface_, &format_count, info.formats.data()));
  }

  /* Present modes */
  uint32_t present_mode_count;
  VULKAN_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface_, &present_mode_count, nullptr));

  if (present_mode_count != 0) {
      info.present_modes.resize(present_mode_count);
      VULKAN_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface_, &present_mode_count,
                                                            info.present_modes.data()));
  }

  return info;
}

}  // namespace liger::rhi