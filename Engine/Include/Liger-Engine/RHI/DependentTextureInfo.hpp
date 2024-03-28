/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DependentTextureInfo.hpp
 * @date 2024-02-25
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

#include <Liger-Engine/RHI/Texture.hpp>

namespace liger::rhi {

namespace detail {

template <typename T, typename DependencyT>
class DependentTextureValue {
 public:
  DependentTextureValue() = default;
  explicit(false) DependentTextureValue(T independent_value);  // NOLINT(google-explicit-constructor)

  T    Get() const;
  bool IsDependent() const;

  void SetIndependent(T independent_value);

  void        SetDependency(DependencyT dependency);
  DependencyT GetDependency() const;

  void UpdateDependentValue(T dependent_value);  ///< Used by RenderGraph

 private:
  T                          independent_value_{};
  T                          dependent_value_{};  ///< Set and updated by render graph
  std::optional<DependencyT> dependency_{std::nullopt};
};

};  // namespace detail

template <typename DependencyT>
struct DependentTextureInfo {
  DependentTextureInfo() = default;
  explicit(false) DependentTextureInfo(const ITexture::Info& info);  // NOLINT(google-explicit-constructor)

  ITexture::Info Get() const;

  /** [Dependent] texture format. */
  detail::DependentTextureValue<Format, DependencyT> format{Format::Invalid};

  /** Type of the texture. */
  TextureType type{TextureType::Texture2D};

  /** Bitmask of all possible usages of the texture which will be needed. */
  DeviceResourceState usage{DeviceResourceState::Undefined};

  /** Whether any views of the texture can be @ref TextureViewType::Cube or @ref TextureViewType::ArrayCube. */
  bool cube_compatible{false};

  /**
   * @brief [Dependent] extent of the texture in pixels.
   * @note extent.z is either depth of the texture if it is 3D, or array size if it is 1D or 2D
   */
  detail::DependentTextureValue<Extent3D, DependencyT> extent{};

  /**
   * @brief [Dependent] number of mip levels in the texture.
   * @warning Must be greater than 0.
   */
  detail::DependentTextureValue<uint32_t, DependencyT> mip_levels{1};

  /**
   * @brief [Dependent] Number of samples (for multi-sampling).
   * @warning Must be greater than 0.
   * @warning Must be less or equal to @ref IDevice::Properties::max_msaa_samples.
   * @warning Must be power of 2, i.e. 1, 2, 4, 8 etc.
   */
  detail::DependentTextureValue<uint8_t, DependencyT> samples{1};

  /** Name of the texture, used mainly for debugging purposes. */
  std::string name;
};

namespace detail {

template <typename T, typename DependencyT>
DependentTextureValue<T, DependencyT>::DependentTextureValue(T independent_value)
    : independent_value_(independent_value) {}

template <typename T, typename DependencyT>
T DependentTextureValue<T, DependencyT>::Get() const {
  return IsDependent() ? dependent_value_ : independent_value_;
}

template <typename T, typename DependencyT>
bool DependentTextureValue<T, DependencyT>::IsDependent() const {
  return dependency_.has_value();
}

template <typename T, typename DependencyT>
void DependentTextureValue<T, DependencyT>::SetIndependent(T independent_value) {
  dependency_        = std::nullopt;
  independent_value_ = independent_value;
}

template <typename T, typename DependencyT>
void DependentTextureValue<T, DependencyT>::SetDependency(DependencyT dependency) {
  dependency_ = dependency;
}

template <typename T, typename DependencyT>
DependencyT DependentTextureValue<T, DependencyT>::GetDependency() const {
  return dependency_.value();
}

template <typename T, typename DependencyT>
void DependentTextureValue<T, DependencyT>::UpdateDependentValue(T dependent_value) {
  dependent_value_ = dependent_value;
}

}  // namespace detail

template <typename DependencyT>
DependentTextureInfo<DependencyT>::DependentTextureInfo(const ITexture::Info& info)
    : format(info.format),
      type(info.type),
      usage(info.usage),
      cube_compatible(info.cube_compatible),
      extent(info.extent),
      mip_levels(info.mip_levels),
      samples(info.samples),
      name(info.name) {}

template <typename DependencyT>
ITexture::Info DependentTextureInfo<DependencyT>::Get() const {
  return ITexture::Info {
    .format          = format.Get(),
    .type            = type,
    .usage           = usage,
    .cube_compatible = cube_compatible,
    .extent          = extent.Get(),
    .mip_levels      = mip_levels.Get(),
    .samples         = samples.Get(),
    .name            = name
  };
}

};  // namespace liger::rhi