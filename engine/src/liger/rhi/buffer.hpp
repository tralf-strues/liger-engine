/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file buffer.hpp
 * @date 2024-01-06
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

#include <liger/rhi/descriptor_binding.hpp>
#include <liger/rhi/device_resource_state.hpp>

#include <string>

namespace liger::rhi {

class IBuffer {
 public:
  struct Info {
    /** Buffer size in bytes. */
    uint64_t size;

    /** Bitmask of all possible usages of the buffer which will be needed. */
    DeviceResourceState usage;

    /**
     * Whether buffer's memory can be mapped on the CPU.
     * @warning Affects performance! Use it with caution!
     */
    bool cpu_visible;

    /** Name of the buffer, used mainly for debugging purposes. */
    std::string name;
  };

  virtual ~IBuffer() = default;

  const Info& GetInfo() const { return info_; }

  /**
   * @brief Get the descriptor index of the buffer for accessing inside shaders as a uniform buffer.
   *
   * @warning This function may return @ref BufferDescriptorBinding::kInvalid if the @ref Info::usage mask
   * did not contain @ref DeviceResourceState::kUniformBuffer bit.
   *
   * @return Uniform buffer's descriptor index.
   */
  virtual BufferDescriptorBinding GetUniformDescriptorBinding() const = 0;

  /**
   * @brief Get the descriptor index of the buffer for accessing inside shaders as a storage buffer.
   *
   * @warning This function may return @ref BufferDescriptorBinding::kInvalid if the @ref Info::usage mask
   * did not contain @ref DeviceResourceState::kStorageBuffer bit.
   *
   * @return Storage buffer's descriptor index.
   */
  virtual BufferDescriptorBinding GetStorageDescriptorBinding() const = 0;

  /**
   * @brief Map a range of buffer's memory.
   *
   * @warning Only available if the buffer is created with @ref Info::cpu_visible enabled.
   *
   * @param offset
   * @param size
   *
   * @return Address of the mapped memory or nullptr in case of error.
   */
  virtual void* MapMemory(uint64_t offset, uint64_t size) = 0;

  /**
   * @brief Unmap buffer's memory.
   *
   * @warning It is UB when calling this method without previously successful call to @ref MapMemory.
   */
  virtual void UnmapMemory() = 0;

 protected:
  IBuffer() = default;
  explicit IBuffer(Info info) : info_(std::move(info)) {}

 private:
  Info info_{};
};

}  // namespace liger::rhi