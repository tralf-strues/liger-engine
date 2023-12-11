/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file swapchain.hpp
 * @date 2023-11-06
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

#include <liger/render/rhi/command_queue.hpp>
#include <liger/render/rhi/device_resource.hpp>
#include <liger/render/rhi/surface.hpp>
#include <liger/render/rhi/texture.hpp>

namespace liger::rhi {

class Swapchain final : DeviceResource {
 public:
  struct Info {
    Surface window_surface;

    /** What swapchain's textures can be used for. */
    TextureUsage usage = TextureUsage::kRenderTarget;

    /**
     * Bitmask of command types that can be used with swapchain's textures.
     * @warning Must contain @ref CommandTypes::kPresent!
     */
    CommandTypes cmd_types_allowed = CommandTypes::kPresent;

    /** Swapchain size, i.e. the number of swapchain's textures. */
    uint8_t size = 2;

    /** Whether vertical synchronization is enabled. */
    bool vsync = true;
  };

 public:
  Swapchain() = default;

  explicit Swapchain(const Info& info, DeviceResource::InternalHandle internal);
  ~Swapchain() override = default;

  const Info& GetInfo() const;

 private:
  Info info_;
};

Swapchain::Swapchain(const Info& info, DeviceResource::InternalHandle internal)
    : DeviceResource(internal), info_(info) {}

const Swapchain::Info& Swapchain::GetInfo() const { return info_; }

}  // namespace liger::rhi