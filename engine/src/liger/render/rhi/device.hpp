/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file device.hpp
 * @date 2023-12-10
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

#include <string>

namespace liger::rhi {

/**
 * @brief Logical device, an interface for working with a physical device (e.g. GPU).
 */
class IDevice {
 public:
  /**
   * @brief Type of the device.
   */
  enum class Type : uint8_t {
    kUndefined,
    kIntegratedGPU,
    kDiscreteGPU,
    kVirtualGPU,
    kCPU,
  };

  /**
   * @brief Properties, features and limits of the device.
   */
  struct Properties {
    bool    sampler_anisotropy;
    uint8_t max_msaa_samples;
    float   max_sampler_anisotropy;
  };

  /**
   * @brief Device info, which can be used to identify the device needed.
   */
  struct Info {
    uint32_t    id;
    std::string name;
    Type        type;
    Properties  properties;
  };

 private:
};

}  // namespace liger::rhi