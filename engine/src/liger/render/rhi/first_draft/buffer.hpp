/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file buffer.hpp
 * @date 2023-09-26
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

#include <liger/core/enum_bitmask.hpp>
#include <liger/render/rhi/device_resource.hpp>

#include <string>

namespace liger::rhi {

enum class BufferUsage : uint32_t {
  kNone          = 0,
  kTransferSrc   = 0x0000'0001,
  kTransferDst   = 0x0000'0002,
  kUniformBuffer = 0x0000'0010,
  kStorageBuffer = 0x0000'0020,
  kIndexBuffer   = 0x0000'0040,
  kVertexBuffer  = 0x0000'0080,
};

class Buffer final : public DeviceResource {
 public:
  struct Info {
    /** Buffer size in bytes. */
    uint64_t size;

    /** Bitmask of all possible usages of the buffer which will be needed. */
    BufferUsage usage;

    /**
     * Whether buffer's memory is visible from CPU.
     * @warning Affects performance! Use it with caution!
     */
    bool cpu_visible;

    /** Name of the buffer, used mainly for debugging purposes. */
    std::string name;
  };

 public:
  Buffer() = default;

  Buffer(const Info& info, DeviceResource::InternalHandle internal);
  ~Buffer() override = default;

  const Info& GetInfo() const;

  void* GetMappedData();
  uint64_t GetMappedSize() const;

  void OnMapped(void* mapped_data, uint64_t mapped_size);

 private:
  Info     info_{};
  void*    mapped_data_{nullptr};
  uint64_t mapped_size_{0};
};

Buffer::Buffer(const Info& info, DeviceResource::InternalHandle internal) : DeviceResource(internal), info_(info) {}

const Buffer::Info& Buffer::GetInfo() const { return info_; }
void*               Buffer::GetMappedData() { return mapped_data_; }
uint64_t            Buffer::GetMappedSize() const { return mapped_size_; }

void Buffer::OnMapped(void* mapped_data, uint64_t mapped_size) {
  mapped_data_ = mapped_data;
  mapped_size_ = mapped_size;
}

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::BufferUsage);
