/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_instance.cpp
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

#include <GLFW/glfw3.h>

#include <liger/core/enum_reflection.hpp>
#include <liger/render/rhi/vulkan/vulkan_instance.hpp>
#include <liger/render/rhi/vulkan/vulkan_utils.hpp>

#include <array>

namespace liger::rhi {

bool CheckValidationLayerSupport() {
  uint32_t layer_count = 0;
  VULKAN_CALL(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

  std::vector<VkLayerProperties> available_layers(layer_count);
  VULKAN_CALL(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

  bool layer_found = false;

  for (const auto& layer_properties : available_layers) {
    if (strcmp(layer_properties.layerName, kValidationLayerName) == 0) {
      layer_found = true;
      break;
    }
  }

  return layer_found;
}

std::vector<const char*> GetInstanceExtensions() {
  std::vector<const char*> extensions;

  /* GLFW extensions */
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  for (uint32_t i = 0; i < glfw_extension_count; ++i) {
    extensions.push_back(glfw_extensions[i]);
  }

#ifdef __APPLE__
  extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

  extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

  return extensions;
}

VulkanInstance::~VulkanInstance() {
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

bool VulkanInstance::Init(ValidationLevel validation) {
  LIGER_LOG_INFO(kLogChannelRHI, "Initializing VulkanInstance with validation={0}", EnumToString(validation));

  if (validation != IInstance::ValidationLevel::kNone && !CheckValidationLayerSupport()) {
    validation = IInstance::ValidationLevel::kNone;
    LIGER_LOG_ERROR(kLogChannelRHI, "Validation layer \"{0}\" is not found", kValidationLayerName);
  }

  const VkApplicationInfo app_info{
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext              = nullptr,
    .pApplicationName   = nullptr,
    .applicationVersion = 0,
    .pEngineName        = "Liger Engine",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_3
  };

  auto extensions = GetInstanceExtensions();

  VkInstanceCreateInfo instance_info{
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext                   = nullptr,
    .flags                   = 0,
    .pApplicationInfo        = &app_info,
    .enabledLayerCount       = 0,
    .ppEnabledLayerNames     = nullptr,
    .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data()
  };

#ifdef __APPLE__
  instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  if (validation != IInstance::ValidationLevel::kNone) {
    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = &kValidationLayerName;
  }

  const VkValidationFeatureEnableEXT extra_features[] = {
    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
  };

  VkValidationFeaturesEXT features_info{
    .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    .pNext                          = nullptr,
    .enabledValidationFeatureCount  = std::size(extra_features),
    .pEnabledValidationFeatures     = extra_features,
    .disabledValidationFeatureCount = 0,
    .pDisabledValidationFeatures    = nullptr
  };

  if (validation == IInstance::ValidationLevel::kExtensive) {
    instance_info.pNext = &features_info;
  }

  VULKAN_CALL(vkCreateInstance(&instance_info, nullptr, &instance_));

  return FillDeviceInfoList();
}

std::span<const IDevice::Info> VulkanInstance::GetDeviceInfoList() const {
  return device_info_list_;
}

std::unique_ptr<IDevice> VulkanInstance::CreateDevice(uint32_t id, uint32_t frames_in_flight) {
  LIGER_LOG_INFO(kLogChannelRHI, "Requesting VulkanDevice with id={0}, configured frames-in-flight={1}", id,
                 frames_in_flight);

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

  const uint32_t device_count = physical_device_ids_.size();
  for (uint32_t i = 0; i < device_count; ++i) {
    if (physical_device_ids_[i] == id) {
      physical_device = physical_devices_[i];
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    LIGER_LOG_ERROR(kLogChannelRHI, "VulkanDevice with id={0} cannot be found!", id);
    return nullptr;
  }

  // TODO (tralf-strues): return device

  return nullptr;
}

bool VulkanInstance::FillDeviceInfoList() {
  uint32_t device_count = 0;
  VULKAN_CALL(vkEnumeratePhysicalDevices(instance_, &device_count, nullptr));
  
  if (device_count == 0) {
    LIGER_LOG_ERROR(kLogChannelRHI, "There are no Vulkan physical devices found!");
    return false;
  }

  physical_devices_.resize(device_count);
  VULKAN_CALL(vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices_.data()));

  physical_device_ids_.reserve(device_count);

  for (auto physical_device : physical_devices_) {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physical_device, &features);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    physical_device_ids_.push_back(properties.deviceID);

    uint32_t extensions_count = 0;
    VULKAN_CALL(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr));

    std::vector<VkExtensionProperties> available_extensions{extensions_count};
    VULKAN_CALL(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count,
                                                     available_extensions.data()));

    bool required_extensions_supported = true;
    for (auto required_extension : kRequiredDeviceExtensions) {
      bool found = false;

      for (auto available_extension : available_extensions) {
        if (std::strcmp(required_extension, available_extension.extensionName) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        required_extensions_supported = false;
        break;
      }
    }

    bool swapchain_supported = false;
    for (auto available_extension : available_extensions) {
      if (std::strcmp(available_extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
        swapchain_supported = true;
        break;
      }
    }

    IDevice::Info info {
        .id   = properties.deviceID,
        .name = properties.deviceName,
        .type = GetDeviceTypeFromVulkan(properties.deviceType),
        .engine_supported =
            required_extensions_supported && swapchain_supported && static_cast<bool>(features.samplerAnisotropy),

        .properties = {
          // .swapchain              = swapchain_supported,
          // .sampler_anisotropy     = static_cast<bool>(features.samplerAnisotropy),
          .max_msaa_samples       = GetMaxSamplesFromVulkan(properties),
          .max_sampler_anisotropy = properties.limits.maxSamplerAnisotropy,
        },
    };

    device_info_list_.emplace_back(std::move(info));
  }

  return true;
}

}  // namespace liger::rhi
