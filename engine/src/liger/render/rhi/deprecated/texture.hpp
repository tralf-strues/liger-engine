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

#include <liger/render/rhi/format.hpp>

namespace liger::rhi {

/* Texture description */
enum class TextureType : uint32 {
  kTexture2D,
  kTextureCube,
  kTexture2DArray
};

enum class CubeMapFaceLayer : uint32 {
  kRightPositiveX  = 0,
  kLeftNegativeX   = 1,

  kTopPositiveY    = 2,
  kBottomNegativeY = 3,

  kFrontPositiveZ  = 4,
  kBackNegativeZ   = 5
};

enum class TextureUsageBit : uint32 {
  kTransferSrc     = 0x0000'0001,
  kTransferDst     = 0x0000'0002,
  kSampled         = 0x0000'0004,
  kColorAttachment = 0x0000'0010,
  kDepthAttachment = 0x0000'0020,
};

using TextureUsage = EnumBitMask<TextureUsageBit, uint32>;

DECLARE_ENUM_CLASS(TextureLayout,
                   uint32,
                   kUndefined,
                   kGeneral,
                   kPresentSrc,

                   kColorAttachment,
                   kDepthStencilAttachment,
                   kDepthStencilReadOnly,
                   kShaderReadOnly,

                   kTransferSrc,
                   kTransferDst);

struct TextureDescription {
  Format       format       {Format::kInvalid};
  TextureType  type         {TextureType::kTexture2D};
  TextureUsage usage        {TextureUsageBit::kSampled};
  bool         cpu_readable {false};  ///< Allows CPU to read texels from it (may affect performance).

  uint32       width        {0};
  uint32       height       {0};
  uint32       mip_levels   {1};
  uint32       samples      {1};

  uint32       array_layers {1};  ///< Used if type is TextureType::kTexture2DArray
};

struct TextureSubresourceDescription {
  uint32 first_mip;
  uint32 mip_count;

  uint32 first_layer;
  uint32 layer_count;
};

/* Texture sampler */
enum class SamplerFilter : uint32 {
  kNearest,
  kLinear
};

enum class SamplerAddressMode : uint32 {
  kRepeat,
  kMirroredRepeat,
  kClampToEdge,
  kClampToBorder,
};

enum class SamplerMipmapMode : uint32 {
  kNearest,
  kLinear
};

enum class SamplerBorderColor : uint32 {
  kFloatTransparentBlack,
  kIntTransparentBlack,
  
  kFloatOpaqueBlack,
  kIntOpaqueBlack,

  kFloatOpaqueWhite,
  kIntOpaqueWhite
};

struct SamplerDescription {
  SamplerFilter      min_filter         {SamplerFilter::kLinear};
  SamplerFilter      mag_filter         {SamplerFilter::kLinear};

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

}  // namespace liger::rhi