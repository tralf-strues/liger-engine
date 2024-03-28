/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanBuffer.cpp
 * @date 2024-02-08
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

#include "VulkanBuffer.hpp"

#include "VulkanDevice.hpp"

namespace liger::rhi {

VulkanBuffer::VulkanBuffer(Info info, VulkanDevice& device) : IBuffer(std::move(info)), device_(device) {}

VulkanBuffer::~VulkanBuffer() {
  if (buffer_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(device_.GetAllocator(), buffer_, allocation_);
  }

  device_.GetDescriptorManager().RemoveBuffer(bindings_);
}

bool VulkanBuffer::Init() {
  VkBufferCreateInfo create_info{
    .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext                 = nullptr,
    .flags                 = 0,
    .size                  = GetInfo().size,
    .usage                 = GetVulkanBufferUsage(GetInfo().usage),
    .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices   = nullptr
  };

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = (GetInfo().cpu_visible ? VMA_MEMORY_USAGE_AUTO : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  alloc_info.flags |= (GetInfo().cpu_visible ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0);

  VULKAN_CALL(vmaCreateBuffer(device_.GetAllocator(), &create_info, &alloc_info, &buffer_, &allocation_, nullptr));

  bindings_ = device_.GetDescriptorManager().AddBuffer(buffer_, GetInfo().usage);

  if (!GetInfo().name.empty()) {
    device_.SetDebugName(buffer_, GetInfo().name);
  }

  return true;
}

BufferDescriptorBinding VulkanBuffer::GetUniformDescriptorBinding() const { return bindings_.uniform; }
BufferDescriptorBinding VulkanBuffer::GetStorageDescriptorBinding() const { return bindings_.storage; }

void* VulkanBuffer::MapMemory(uint64_t offset, uint64_t /*size*/) {
  void* map_data{nullptr};
  VULKAN_CALL(vmaMapMemory(device_.GetAllocator(), allocation_, &map_data));

  return static_cast<void*>(static_cast<uint8_t*>(map_data) + offset);
}

void VulkanBuffer::UnmapMemory() { vmaUnmapMemory(device_.GetAllocator(), allocation_); }

VkBuffer VulkanBuffer::GetVulkanBuffer() const {
  return buffer_;
}

}  // namespace liger::rhr