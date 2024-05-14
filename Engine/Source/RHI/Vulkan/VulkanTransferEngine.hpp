/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file VulkanTransferEngine.hpp
 * @date 2024-05-14
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

#include "VulkanUtils.hpp"

#include <list>

namespace liger::rhi {

class VulkanDevice;

class VulkanTransferEngine {
 public:
  explicit VulkanTransferEngine(VulkanDevice& device);
  ~VulkanTransferEngine();

  void Init(VkQueue queue, uint32_t queue_family, uint64_t staging_capacity);

  void Request(IDevice::DedicatedTransferRequest&& transfer);

  void SubmitAndWait();

 private:
  void Flip();
  void ReschedulePending();

  VulkanDevice&                                device_;
  VkQueue                                      queue_{VK_NULL_HANDLE};

  VkCommandPool                                command_pools_[2U]{VK_NULL_HANDLE, VK_NULL_HANDLE};

  VkBuffer                                     staging_buffers_[2U]{VK_NULL_HANDLE, VK_NULL_HANDLE};
  VmaAllocation                                allocations_[2U]{VK_NULL_HANDLE, VK_NULL_HANDLE};
  uint64_t                                     staging_capacity_{0U};

  VkCommandBuffer                              cur_cmds_{VK_NULL_HANDLE};
  uint32_t                                     cur_idx_{0U};
  std::vector<IDevice::TransferCallback>       cur_callbacks_;
  void*                                        cur_mapped_data_{nullptr};
  uint64_t                                     cur_data_size_{0U};

  std::list<IDevice::DedicatedTransferRequest> pending_;
};

}  // namespace liger::rhi