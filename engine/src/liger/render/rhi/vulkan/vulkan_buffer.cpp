/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_buffer.cpp
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

#include <liger/render/rhi/vulkan/vulkan_buffer.hpp>
#include <liger/render/rhi/vulkan/vulkan_utils.hpp>

namespace liger::rhi {

VulkanBuffer::VulkanBuffer(Info info, VmaAllocator vma_allocator)
    : IBuffer(std::move(info)), vma_allocator_(vma_allocator) {}

VulkanBuffer::~VulkanBuffer() {
  if (vk_buffer_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vma_allocator_, vk_buffer_, vma_allocation_);
  }
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

  VULKAN_CALL(vmaCreateBuffer(vma_allocator_, &create_info, &alloc_info, &vk_buffer_, &vma_allocation_, nullptr));

  return true;
}

uint32_t VulkanBuffer::GetBinding() {
  // TODO(tralf-strues): IMPLEMENT
  return 0;
}

void* VulkanBuffer::MapMemory(uint64_t offset, uint64_t /*size*/) {
  void* map_data{nullptr};
  VULKAN_CALL(vmaMapMemory(vma_allocator_, vma_allocation_, &map_data));

  return static_cast<void*>(static_cast<uint8_t*>(map_data) + offset);
}

void VulkanBuffer::UnmapMemory() {
  vmaUnmapMemory(vma_allocator_, vma_allocation_);
}

}  // namespace liger::rhr