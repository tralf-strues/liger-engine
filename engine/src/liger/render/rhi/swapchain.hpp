/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file swapchain.hpp
 * @date 2024-02-03
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

#include <liger/core/platform/window.hpp>
#include <liger/render/rhi/texture.hpp>

#include <span>

namespace liger::rhi {

class ISwapchain {
 public:
  struct Info {
    /** Target window. It must be valid for the lifetime of swapchain. */
    Window* window = nullptr;

    /** The number of swapchain textures. */
    uint8_t size = 2;

    /** Whether vertical synchronization is enabled. */
    bool vsync = true;

    /** What swapchain textures can be used for. */
    DeviceResourceState usage = DeviceResourceState::kColorTarget;
  };

 public:
  virtual ~ISwapchain() = default;

  const Info& GetInfo() const;

  /**
   * @brief Get the swapchain textures.
   *
   * @note The textures are owned by the swapchain, so they get deleted automatically when the swapchain is deleted or
   * recreated.
   *
   * @return Swapchain textures.
   */
  virtual std::span<ITexture*> GetTextures() = 0;

  /**
   * @brief Recreate the swapchain.
   * @note It's recommended to recreate a swapchain using this method instead of deleting and creating a new one.
   * @note After recreating the swapchain, one should retrieve the textures once more.
   */
  virtual void Recreate() = 0;

 protected:
  ISwapchain() = default;
  explicit ISwapchain(const Info& info);

 private:
  Info info_{};
};

}  // namespace liger::rhi