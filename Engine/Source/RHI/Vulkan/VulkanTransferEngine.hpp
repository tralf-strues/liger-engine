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

#include "VulkanCommandPool.hpp"
#include "VulkanTimelineSemaphore.hpp"

#include <list>

namespace liger::rhi {

class VulkanDevice;

class VulkanTransferEngine {
 public:
  explicit VulkanTransferEngine(VulkanDevice& device);
  ~VulkanTransferEngine();

  void Init(uint64_t staging_capacity);

  void Request(IDevice::DedicatedTransferRequest&& transfer);

  void Submit();

 private:
  void Flip(uint32_t next_frame);
  void ReschedulePending();

  void ProcessBufferTransfers(IDevice::DedicatedTransferRequest& transfer);
  void ProcessTextureTransfers(IDevice::DedicatedTransferRequest& transfer);

  struct Callback {
    IDevice::TransferCallback callback;
    uint64_t                  semaphore_value;
  };

  struct StagingBuffer {
    VkBuffer      buffer;
    VmaAllocation allocation;
  };

  VulkanDevice&                                device_;
  VulkanTimelineSemaphore                      timeline_semaphore_;

  VulkanCommandPool                            command_pool_;
  VulkanCommandBuffer                          cmds_transfer_;
  VulkanCommandBuffer                          cmds_graphics_;

  std::vector<StagingBuffer>                   staging_buffers_;
  uint64_t                                     staging_capacity_{0U};

  uint32_t                                     cur_frame_;
  void*                                        cur_mapped_data_{nullptr};
  uint64_t                                     cur_data_size_{0U};

  std::list<Callback>                          callbacks_;
  std::list<IDevice::DedicatedTransferRequest> pending_;
};

}  // namespace liger::rhi