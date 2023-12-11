/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file sampler.hpp
 * @date 2023-11-04
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

#include <liger/render/rhi/device_resource.hpp>

namespace liger::rhi {

enum class Filter : uint8_t {
  kNearest,
  kLinear
};

enum class SamplerAddressMode : uint8_t {
  kRepeat,
  kMirroredRepeat,
  kClampToEdge,
  kClampToBorder,
};

enum class SamplerMipmapMode : uint8_t {
  kNearest,
  kLinear
};

enum class SamplerBorderColor : uint8_t {
  kFloatTransparentBlack,
  kIntTransparentBlack,
  
  kFloatOpaqueBlack,
  kIntOpaqueBlack,

  kFloatOpaqueWhite,
  kIntOpaqueWhite
};

class Sampler final : DeviceResource {
 public:
  struct Info {
    Filter             min_filter         {Filter::kLinear};
    Filter             mag_filter         {Filter::kLinear};

    SamplerAddressMode address_mode_u     {SamplerAddressMode::kRepeat};
    SamplerAddressMode address_mode_v     {SamplerAddressMode::kRepeat};
    SamplerAddressMode address_mode_w     {SamplerAddressMode::kRepeat};
    SamplerBorderColor border_color       {SamplerBorderColor::kIntOpaqueBlack};

    bool               anisotropy_enabled {false};
    float              max_anisotropy     {0.0f};

    SamplerMipmapMode  mipmap_mode        {SamplerMipmapMode::kLinear};
    float              min_lod            {0.0f};
    float              max_lod            {0.0f};
    float              lod_bias           {0.0f};
  };

 public:
  Sampler() = default;

  Sampler(const Info& info, DeviceResource::InternalHandle internal);
  ~Sampler() override = default;

  const Info& GetInfo() const;

 private:
  Info info_{};
};

Sampler::Sampler(const Info& info, DeviceResource::InternalHandle internal) : DeviceResource(internal), info_(info) {}

const Sampler::Info& Sampler::GetInfo() const { return info_; }

}  // namespace liger::rhi