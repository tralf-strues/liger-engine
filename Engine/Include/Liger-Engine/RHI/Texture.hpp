/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Texture.hpp
 * @date 2024-01-02
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

#include <Liger-Engine/RHI/DescriptorBinding.hpp>
#include <Liger-Engine/RHI/DeviceResourceState.hpp>
#include <Liger-Engine/RHI/Extent.hpp>
#include <Liger-Engine/RHI/Format.hpp>
#include <Liger-Engine/RHI/SamplerInfo.hpp>

#include <string>

namespace liger::rhi {

constexpr uint32_t kTextureDefaultViewIdx = 0;

enum class TextureType : uint8_t {
  Texture1D,
  Texture2D,
  Texture3D,
};

enum class TextureViewType : uint8_t {
  View1D,
  View2D,
  View3D,

  Cube,

  Array1D,
  Array2D,
  ArrayCube,
};

enum class CubeMapFaceLayer : uint8_t {
  RightPositiveX  = 0,
  LeftNegativeX   = 1,

  TopPositiveY    = 2,
  BottomNegativeY = 3,

  FrontPositiveZ  = 4,
  BackNegativeZ   = 5
};

enum class TextureLayout : uint32_t {
  Undefined,
  General,

  PresentSrc,

  Write,
  ReadOnly,

  TransferSrc,
  TransferDst
};

struct TextureViewInfo {
  TextureViewType type;

  uint32_t first_mip;
  uint32_t mip_count;

  uint32_t first_layer;
  uint32_t layer_count;
};

class ITexture {
 public:
  struct Info {
    /** Texture format. */
    Format format{Format::Invalid};

    /** Type of the texture. */
    TextureType type{TextureType::Texture2D};

    /** Bitmask of all possible usages of the texture which will be needed. */
    DeviceResourceState usage{DeviceResourceState::Undefined};

    /** Whether any views of the texture can be @ref TextureViewType::Cube or @ref TextureViewType::ArrayCube. */
    bool cube_compatible{false};

    /**
     * @brief Extent of the texture in pixels.
     * @note extent.z is either depth of the texture if it is 3D, or array size if it is 1D or 2D
     */
    Extent3D extent{};

    /**
     * @brief Number of mip levels in the texture.
     * @warning Must be greater than 0.
     */
    uint32_t mip_levels{1};

    /**
     * @brief Number of samples (for multi-sampling).
     * @warning Must be greater than 0.
     * @warning Must be less or equal to @ref IDevice::Properties::max_msaa_samples.
     * @warning Must be power of 2, i.e. 1, 2, 4, 8 etc.
     */
    uint8_t samples{1};

    /** Name of the texture, used mainly for debugging purposes. */
    std::string name;
  };

  virtual ~ITexture() = default;

  const Info& GetInfo() const { return info_; }

  /**
   * @brief Create a view for the texture.
   *
   * @note The indexing of views is monotonous. Upon texture creation, a default view is created with index
   * kTextureDefaultViewIdx = 0, and after that each consecutive call to this method returns the next integer.
   *
   * @param info
   * @return View index.
   */
  virtual uint32_t CreateView(const TextureViewInfo& info) = 0;

  virtual bool ViewCreated(uint32_t view) const = 0;

  virtual const TextureViewInfo& GetViewInfo(uint32_t view) const = 0;

  /**
   * @brief Get the sampled binding of the texture's view for accessing inside shaders.
   *
   * @warning This function may return @ref TextureDescriptorBinding::Invalid if the @ref Info::usage mask
   * did not contain @ref DeviceResourceState::ShaderSampled bit.
   *
   * @param view View index. Must be created before calling this method.
   *
   * @return View's binding index.
   */
  virtual TextureDescriptorBinding GetSampledDescriptorBinding(uint32_t view = kTextureDefaultViewIdx) const = 0;

  /**
   * @brief Get the storage binding of the texture's view for accessing inside shaders.
   *
   * @warning This function may return @ref TextureDescriptorBinding::Invalid if the @ref Info::usage mask
   * did not contain @ref DeviceResourceState::StorageTexture bit.
   *
   * @param view View index. Must be created before calling this method.
   *
   * @return View's binding index.
   */
  virtual TextureDescriptorBinding GetStorageDescriptorBinding(uint32_t view = kTextureDefaultViewIdx) const = 0;

  /**
   * @brief Set a custom sampler to the particular texture's view.
   *
   * @warning This function may return false if the @ref Info::usage mask did not
   * contain @ref DeviceResourceState::ShaderSampled bit.
   *
   * @param sampler_info
   * @param view
   *
   * @return Whether sampler has been set successfully.
   */
  virtual bool SetSampler(const SamplerInfo& sampler_info, uint32_t view = kTextureDefaultViewIdx) = 0;

 protected:
  ITexture() = default;
  explicit ITexture(Info info) : info_(std::move(info)) {}

 private:
  Info info_{};
};

}  // namespace liger::rhi