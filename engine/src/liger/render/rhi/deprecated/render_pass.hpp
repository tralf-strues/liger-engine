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

#include <liger/render/rhi/format.hpp>
#include <liger/render/rhi/framebuffer.hpp>
#include <liger/render/rhi/texture.hpp>

namespace liger::rhi {

/* Render Pass Attachment */
DECLARE_ENUM_CLASS(AttachmentLoad, uint32, kLoad, kClear, kDontCare);
DECLARE_ENUM_CLASS(AttachmentStore, uint32, kStore, kDontCare);

DECLARE_ENUM_CLASS(AttachmentType, uint32, kColor, kDepthStencil);

struct AttachmentDescription {
  AttachmentType  type           {AttachmentType::kColor};

  Format          format         {Format::kInvalid};
  uint32          samples        {1};

  AttachmentLoad  load          {AttachmentLoad::kDontCare};
  AttachmentStore store         {AttachmentStore::kDontCare};

  TextureLayout   initial_layout {TextureLayout::kUndefined};
  TextureLayout   usage_layout   {TextureLayout::kUndefined};
  TextureLayout   final_layout   {TextureLayout::kUndefined};
};

/* Render Pass Description */
struct RenderPassDescription {
  /** @note All color attachments should have the same number of samples! **/
  std::array<AttachmentDescription, kMaxFramebufferAttachments> attachments;
};

/* Clear Value */
union ClearColorValue {
  float  rgba_float32[4];
  int32  rgba_int32[4];
  uint32 rgba_uint32[4];

  ClearColorValue() = default;
  ClearColorValue(float r, float g, float b, float a) : rgba_float32{ r, g, b, a } {}
  ClearColorValue(int32 r, int32 g, int32 b, int32 a) : rgba_int32{ r, g, b, a } {}
  ClearColorValue(uint32 r, uint32 g, uint32 b, uint32 a) : rgba_uint32{r, g, b, a} {}
};

struct ClearDepthStencilValue {
  float  depth;
  uint32 stencil;
};

union ClearValue {
  ClearColorValue        color;
  ClearDepthStencilValue depth_stencil;

  ClearValue() = default;

  ClearValue(float r, float g, float b, float a) : color(r, g, b, a) {}
  explicit ClearValue(const glm::vec4& rgba) : ClearValue(rgba.r, rgba.g, rgba.b, rgba.a) {}

  ClearValue(float depth, uint32 stencil) : depth_stencil({depth, stencil}) {}
};

}