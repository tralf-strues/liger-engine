/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file texture.hpp
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
#include <liger/render/rhi/extent.hpp>
#include <liger/render/rhi/format.hpp>

#include <string>

namespace liger::rhi {

constexpr uint32_t kTextureDefaultViewIdx = 0;

enum class TextureType : uint8_t {
  kTexture1D,
  kTexture2D,
  kTexture3D,
};

enum class TextureViewType : uint8_t {
  k1D,
  k2D,
  k3D,

  kCube,

  kArray1D,
  kArray2D,
  kArrayCube,
};

enum class CubeMapFaceLayer : uint8_t {
  kRightPositiveX  = 0,
  kLeftNegativeX   = 1,

  kTopPositiveY    = 2,
  kBottomNegativeY = 3,

  kFrontPositiveZ  = 4,
  kBackNegativeZ   = 5
};

enum class TextureUsage : uint32_t {
  kNone               = 0,
  kTransferSrc        = 0x0000'0001,
  kTransferDst        = 0x0000'0002,
  kSampled            = 0x0000'0004,
  kRenderTarget       = 0x0000'0010,
  kDepthStencilBuffer = 0x0000'0020,
};

enum class TextureLayout : uint32_t {
  kUndefined,
  kGeneral,

  kPresentSrc,

  kWrite,
  kReadOnly,

  kTransferSrc,
  kTransferDst
};

struct TextureViewInfo {
  TextureViewType type;

  uint32_t first_mip;
  uint32_t mip_count;

  uint32_t first_layer;
  uint32_t layer_count;
};

class Texture final : DeviceResource {
 public:
  struct Info {
    /** Texture format. */
    Format format{Format::kInvalid};

    /** Type of the texture. */
    TextureType type{TextureType::kTexture2D};

    /** Bitmask of all possible usages of the texture which will be needed. */
    TextureUsage usage{TextureUsage::kNone};

    /**
     * Extent of the texture in pixels
     * @note extent.z is either depth of the texture if it is 3D, or array size if it is 1D or 2D
     */
    Extent3D extent{};

    /**
     * Number of mip levels in the texture.
     * @warning Must be greater than 0.
     */
    uint32_t mip_levels{1};

    /**
     * Number of samples (for multi-sampling).
     * @warning Must be greater than 0.
     * @warning Must be less or equal to @ref DeviceInfo::max_msaa_samples.
     */
    uint8_t samples{1};

    /** Name of the texture, used mainly for debugging purposes. */
    std::string name;
  };

 public:
  Texture() = default;

  Texture(const Info& info, DeviceResource::InternalHandle internal);
  ~Texture() override = default;

  const Info& GetInfo() const;

 private:
  Info info_{};
};

Texture::Texture(const Info& info, DeviceResource::InternalHandle internal) : DeviceResource(internal), info_(info) {}

const Texture::Info& Texture::GetInfo() const { return info_; }

}  // namespace liger::rhi

ENABLE_ENUM_BITMASK(liger::rhi::TextureUsage);