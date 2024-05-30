/**
* @author Nikita Mochalov (github.com/tralf-strues)
* @file MappedBuffer.hpp
* @date 2024-05-11
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
#include <Liger-Engine/RHI/LogChannel.hpp>

namespace liger::rhi {

template <typename T>
class UniqueMappedBuffer {
 public:
  UniqueMappedBuffer() = default;

  UniqueMappedBuffer(IDevice& device, DeviceResourceState usage, std::string_view name = "", uint32_t count = 1U);
  ~UniqueMappedBuffer();

  UniqueMappedBuffer(const UniqueMappedBuffer& rhs)            = delete;
  UniqueMappedBuffer& operator=(const UniqueMappedBuffer& rhs) = delete;

  UniqueMappedBuffer(UniqueMappedBuffer&& rhs)            = default;
  UniqueMappedBuffer& operator=(UniqueMappedBuffer&& rhs) = default;

  T* GetData();
  const T* GetData() const;

  IBuffer*       get()              { return buffer_.get(); }
  const IBuffer* get() const        { return buffer_.get(); }
  IBuffer*       operator->()       { return get(); }
  const IBuffer* operator->() const { return get(); }

  operator bool() { return buffer_.operator bool(); }

 private:
  std::unique_ptr<IBuffer> buffer_{nullptr};
  T*                       mapped_data_{nullptr};
};

template <typename T>
class SharedMappedBuffer {
 public:
  SharedMappedBuffer() = default;

  SharedMappedBuffer(IDevice& device, DeviceResourceState usage, std::string_view name = "", uint32_t count = 1U);
  ~SharedMappedBuffer();

  SharedMappedBuffer(const SharedMappedBuffer& rhs)            = default;
  SharedMappedBuffer& operator=(const SharedMappedBuffer& rhs) = default;

  SharedMappedBuffer(SharedMappedBuffer&& rhs)            = default;
  SharedMappedBuffer& operator=(SharedMappedBuffer&& rhs) = default;

  T* GetData();
  const T* GetData() const;

  IBuffer*       get()              { return buffer_.get(); }
  const IBuffer* get() const        { return buffer_.get(); }
  IBuffer*       operator->()       { return get(); }
  const IBuffer* operator->() const { return get(); }

  operator bool() { return buffer_.operator bool(); }

 private:
  std::shared_ptr<IBuffer> buffer_{nullptr};
  T*                       mapped_data_{nullptr};
};

template <typename T>
UniqueMappedBuffer<T>::UniqueMappedBuffer(IDevice& device, DeviceResourceState usage, std::string_view name, uint32_t count) {
  buffer_ = device.CreateBuffer(IBuffer::Info {
    .size        = count * sizeof(T),
    .usage       = usage,
    .cpu_visible = true,
    .name        = std::string(name)
  });

  mapped_data_ = reinterpret_cast<T*>(buffer_->MapMemory());
}

template <typename T>
UniqueMappedBuffer<T>::~UniqueMappedBuffer() {
  if (buffer_) {
    buffer_->UnmapMemory();
  }
}

template <typename T>
T* UniqueMappedBuffer<T>::GetData() {
  LIGER_ASSERT(mapped_data_ != nullptr, kLogChannelRHI, "Trying to access unmapped data");
  return mapped_data_;
}

template <typename T>
const T* UniqueMappedBuffer<T>::GetData() const {
  LIGER_ASSERT(mapped_data_ != nullptr, kLogChannelRHI, "Trying to access unmapped data");
  return mapped_data_;
}

template <typename T>
SharedMappedBuffer<T>::SharedMappedBuffer(IDevice& device, DeviceResourceState usage, std::string_view name, uint32_t count) {
  buffer_ = device.CreateBuffer(IBuffer::Info {
    .size        = count * sizeof(T),
    .usage       = usage,
    .cpu_visible = true,
    .name        = std::string(name)
  });

  mapped_data_ = reinterpret_cast<T*>(buffer_->MapMemory());
}

template <typename T>
SharedMappedBuffer<T>::~SharedMappedBuffer() {
  if (buffer_ && buffer_.use_count() == 1) {
    buffer_->UnmapMemory();
  }
}

template <typename T>
T* SharedMappedBuffer<T>::GetData() {
  LIGER_ASSERT(mapped_data_ != nullptr, kLogChannelRHI, "Trying to access unmapped data");
  return mapped_data_;
}

template <typename T>
const T* SharedMappedBuffer<T>::GetData() const {
  LIGER_ASSERT(mapped_data_ != nullptr, kLogChannelRHI, "Trying to access unmapped data");
  return mapped_data_;
}

}  // namespace liger::rhi