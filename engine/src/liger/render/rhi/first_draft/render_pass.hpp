/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file render_pass.hpp
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

#include <liger/core/math/math.hpp>
#include <liger/render/rhi/format.hpp>
#include <liger/render/rhi/framebuffer.hpp>
#include <liger/render/rhi/texture.hpp>

namespace liger::rhi {

/* Clear Value */
union ClearColorValue {
  float    rgba_float32[4];
  int32_t  rgba_int32[4];
  uint32_t rgba_uint32[4];

  ClearColorValue() = default;
  ClearColorValue(float r, float g, float b, float a) : rgba_float32{ r, g, b, a } {}
  ClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a) : rgba_int32{ r, g, b, a } {}
  ClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a) : rgba_uint32{r, g, b, a} {}
};

struct ClearDepthStencilValue {
  float    depth;
  uint32_t stencil;
};

union ClearValue {
  ClearColorValue        color;
  ClearDepthStencilValue depth_stencil;

  ClearValue() = default;

  ClearValue(float r, float g, float b, float a) : color(r, g, b, a) {}
  explicit ClearValue(const glm::vec4& rgba) : ClearValue(rgba.r, rgba.g, rgba.b, rgba.a) {}

  ClearValue(float depth, uint32_t stencil) : depth_stencil({depth, stencil}) {}
};

/* Render Pass Attachment */
enum class AttachmentLoad : uint8_t {
  kLoad,
  kClear,
  kDontCare
};

enum class AttachmentStore : uint8_t {
  kStore,
  kDontCare
};

enum class AttachmentType : uint8_t {
  kRenderTarget,
  kDepthStencilBuffer
};

struct AttachmentInfo {
  AttachmentType  type           {AttachmentType::kRenderTarget};

  Format          format         {Format::kInvalid};
  uint8_t         samples        {1};

  AttachmentLoad  load           {AttachmentLoad::kDontCare};
  AttachmentStore store          {AttachmentStore::kDontCare};

  TextureLayout   initial_layout {TextureLayout::kUndefined};
  TextureLayout   usage_layout   {TextureLayout::kUndefined};
  TextureLayout   final_layout   {TextureLayout::kUndefined};
};

/* Render Pass */
class RenderPass final : DeviceResource {
 public:
  struct Info {
    /** @note All render targets should have the same number of samples! **/
    std::array<AttachmentInfo, Framebuffer::kMaxAttachments> attachments;
  };

 public:
  RenderPass() = default;

  RenderPass(const Info& info, DeviceResource::InternalHandle internal);
  ~RenderPass() override = default;

  const Info& GetInfo() const;

 private:
  Info info_{};
};

RenderPass::RenderPass(const Info& info, DeviceResource::InternalHandle internal) : DeviceResource(internal), info_(info) {}

const RenderPass::Info& RenderPass::GetInfo() const { return info_; }

}  // namespace liger::rhi