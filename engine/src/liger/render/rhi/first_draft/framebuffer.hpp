/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file framebuffer.hpp
 * @date 2023-09-27
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

#include <liger/render/rhi/texture.hpp>

namespace liger::rhi {

struct FramebufferAttachment {
  Texture texture {};
  uint32_t view{0};

  FramebufferAttachment() = default;
  explicit FramebufferAttachment(Texture texture, uint32_t view = 0) : texture(texture), view(view) {}
};

class Framebuffer final : DeviceResource {
 public:
  static constexpr uint32_t kMaxAttachments = 8;

  struct Info {
    /** @note All render targets should have the same number of samples! **/
    std::array<FramebufferAttachment, kMaxAttachments> attachments;
  };

 public:
  Framebuffer() = default;

  Framebuffer(const Info& info, DeviceResource::InternalHandle internal);
  ~Framebuffer() override = default;

  const Info& GetInfo() const;

 private:
  Info info_{};
};

Framebuffer::Framebuffer(const Info& info, DeviceResource::InternalHandle internal)
    : DeviceResource(internal), info_(info) {}

const Framebuffer::Info& Framebuffer::GetInfo() const { return info_; }

}  // namespace liger::rhi