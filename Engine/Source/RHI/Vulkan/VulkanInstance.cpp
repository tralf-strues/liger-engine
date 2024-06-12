/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanInstance.cpp
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

#include "VulkanInstance.hpp"

#include <Liger-Engine/Core/EnumReflection.hpp>
#include "VulkanUtils.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <utility>

namespace liger::rhi {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             /*message_type*/,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                      void*                                       /*user_data*/) {
  switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
      LIGER_LOG_INFO(kLogChannelRHI, "{0} - {1}: {2}", callback_data->messageIdNumber, callback_data->pMessageIdName,
                     callback_data->pMessage);
      break;
    }

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
      LIGER_LOG_WARN(kLogChannelRHI, "{0} - {1}: {2}", callback_data->messageIdNumber, callback_data->pMessageIdName,
                     callback_data->pMessage);
      break;
    }

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
      LIGER_LOG_ERROR(kLogChannelRHI, "{0} - {1}: {2}", callback_data->messageIdNumber, callback_data->pMessageIdName,
                      callback_data->pMessage);
      break;
    }

    default: {}
  }

  return VK_FALSE;
}

bool CheckValidationLayerSupport() {
  uint32_t layer_count = 0;
  VULKAN_CALL(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

  std::vector<VkLayerProperties> available_layers(layer_count);
  VULKAN_CALL(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

  bool layer_found = false;

  for (const auto& layer_properties : available_layers) {
    if (strcmp(layer_properties.layerName, VulkanDevice::kValidationLayerName) == 0) {
      layer_found = true;
      break;
    }
  }

  return layer_found;
}

std::vector<const char*> GetInstanceExtensions(IInstance::ValidationLevel validation) {
  std::vector<const char*> extensions;

  /* GLFW extensions */
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  for (uint32_t i = 0; i < glfw_extension_count; ++i) {
    extensions.push_back(glfw_extensions[i]);
  }

#ifdef __APPLE__
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
#endif

  if (validation != IInstance::ValidationLevel::None) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

VulkanInstance::~VulkanInstance() {
  if (debug_utils_messenger_ != VK_NULL_HANDLE) {
    vkDestroyDebugUtilsMessengerEXT(instance_, debug_utils_messenger_, nullptr);
    debug_utils_messenger_ = VK_NULL_HANDLE;
  }

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

bool VulkanInstance::Init(ValidationLevel validation) {
  LIGER_LOG_INFO(kLogChannelRHI, "Initializing VulkanInstance with validation={0}", EnumToString(validation));

  VULKAN_CALL(volkInitialize());

  if (validation != IInstance::ValidationLevel::None && !CheckValidationLayerSupport()) {
    validation = IInstance::ValidationLevel::None;
    LIGER_LOG_ERROR(kLogChannelRHI, "Validation layer \"{0}\" is not found", VulkanDevice::kValidationLayerName);
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

  auto extensions = GetInstanceExtensions(validation);

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

  constexpr int32_t kUseMetalArgumentBuffers = 1;

  const VkLayerSettingEXT layer_settings[] = {{"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS",
                                               VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &kUseMetalArgumentBuffers}};

  VkLayerSettingsCreateInfoEXT layer_settings_info = {
    .sType        = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
    .pNext        = nullptr,
    .settingCount = std::size(layer_settings),
    .pSettings    = layer_settings
  };
#endif

  VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext = nullptr,
    .flags = 0,
    .messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
    .pfnUserCallback = DebugMessengerCallback,
    .pUserData       = nullptr,
  };

  if (validation != IInstance::ValidationLevel::None && validation != IInstance::ValidationLevel::DebugInfoOnly) {
    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = &VulkanDevice::kValidationLayerName;
  }

  const VkValidationFeatureEnableEXT extra_features[] = {
    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
  };

#ifdef __APPLE__
  const VkValidationFeatureDisableEXT disable_extra_features[] = {
    VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
    VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT
  };
#endif

  VkValidationFeaturesEXT features_info{
    .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    .pNext                          = nullptr,
    .enabledValidationFeatureCount  = std::size(extra_features),
    .pEnabledValidationFeatures     = extra_features,

#ifdef __APPLE__
    .disabledValidationFeatureCount = std::size(disable_extra_features),
    .pDisabledValidationFeatures    = disable_extra_features
#else
    .disabledValidationFeatureCount = 0,
    .pDisabledValidationFeatures    = nullptr
#endif
  };

#ifdef __APPLE__
  if (validation == IInstance::ValidationLevel::Basic) {
    instance_info.pNext       = &layer_settings_info;
    layer_settings_info.pNext = &debug_utils_create_info;
  } else if (validation == IInstance::ValidationLevel::Extensive) {
    instance_info.pNext       = &features_info;
    features_info.pNext       = &layer_settings_info;
    layer_settings_info.pNext = &debug_utils_create_info;
  } else {
    instance_info.pNext = &layer_settings_info;
  }
#else
  if (validation == IInstance::ValidationLevel::Basic) {
    instance_info.pNext = &debug_utils_create_info;
  } else if (validation == IInstance::ValidationLevel::Extensive) {
    features_info.pNext = &debug_utils_create_info;
    instance_info.pNext = &features_info;
  }
#endif

  VULKAN_CALL(vkCreateInstance(&instance_info, nullptr, &instance_));

  volkLoadInstanceOnly(instance_);

  if (validation != IInstance::ValidationLevel::None) {
    VULKAN_CALL(vkCreateDebugUtilsMessengerEXT(instance_, &debug_utils_create_info, nullptr, &debug_utils_messenger_));
  }

  validation_ = validation;

  return FillDeviceInfoList();
}

std::span<const IDevice::Info> VulkanInstance::GetDeviceInfoList() const {
  return device_info_list_;
}

std::unique_ptr<IDevice> VulkanInstance::CreateDevice(uint32_t id, uint32_t frames_in_flight) {
  LIGER_LOG_INFO(kLogChannelRHI, "Requesting VulkanDevice with id={0}, configured frames-in-flight={1}", id,
                 frames_in_flight);

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  uint32_t device_info_index = 0;

  const uint32_t device_count = physical_device_ids_.size();
  for (uint32_t i = 0; i < device_count; ++i) {
    if (physical_device_ids_[i] == id) {
      physical_device = physical_devices_[i];
      device_info_index = i;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    LIGER_LOG_ERROR(kLogChannelRHI, "VulkanDevice with id={0} cannot be found!", id);
    return nullptr;
  }

  auto device = std::make_unique<VulkanDevice>(device_info_list_[device_info_index], frames_in_flight, instance_,
                                               physical_device);
  if (!device->Init(validation_ != IInstance::ValidationLevel::None)) {
    return nullptr;
  }

  return device;
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
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    VkPhysicalDeviceSynchronization2FeaturesKHR sync2_feature {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
      .pNext = nullptr
    };

    VkPhysicalDeviceVulkan12Features features12 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &sync2_feature
    };

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamic_state3_features {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
      .pNext = &features12
    };

    VkPhysicalDeviceFeatures2 features2 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &dynamic_state3_features
    };

    vkGetPhysicalDeviceFeatures2(physical_device, &features2);

    physical_device_ids_.push_back(properties.deviceID);

    uint32_t extensions_count = 0;
    VULKAN_CALL(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr));

    std::vector<VkExtensionProperties> available_extensions{extensions_count};
    VULKAN_CALL(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count,
                                                     available_extensions.data()));

    bool required_extensions_supported = true;
    for (auto required_extension : VulkanDevice::kRequiredDeviceExtensions) {
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
      .id               = properties.deviceID,
      .name             = properties.deviceName,
      .type             = GetDeviceTypeFromVulkan(properties.deviceType),
      .engine_supported = required_extensions_supported && swapchain_supported &&
                          static_cast<bool>(features2.features.samplerAnisotropy) &&
                          static_cast<bool>(features2.features.shaderInt64) &&
                          static_cast<bool>(features2.features.multiDrawIndirect) &&
                          static_cast<bool>(features2.features.drawIndirectFirstInstance) &&
                          static_cast<bool>(dynamic_state3_features.extendedDynamicState3RasterizationSamples) &&
                          static_cast<bool>(features12.descriptorBindingPartiallyBound) &&
                          static_cast<bool>(features12.runtimeDescriptorArray) &&
                          static_cast<bool>(features12.shaderUniformBufferArrayNonUniformIndexing) &&
                          static_cast<bool>(features12.shaderStorageBufferArrayNonUniformIndexing) &&
                          static_cast<bool>(features12.timelineSemaphore) &&
                          static_cast<bool>(features12.bufferDeviceAddress) &&
                          static_cast<bool>(features12.scalarBlockLayout) &&
                          static_cast<bool>(features12.shaderSampledImageArrayNonUniformIndexing) &&
                          static_cast<bool>(features12.shaderStorageImageArrayNonUniformIndexing) &&
                          static_cast<bool>(sync2_feature.synchronization2),

      .properties = {
              .max_msaa_samples       = GetMaxSamplesFromVulkan(properties),
              .max_sampler_anisotropy = properties.limits.maxSamplerAnisotropy,
      },
    };

    device_info_list_.emplace_back(std::move(info));
  }

  return true;
}

}  // namespace liger::rhi
