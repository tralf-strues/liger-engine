/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file graphics_pipeline.hpp
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

#include <liger/render/rhi/format.hpp>
#include <liger/render/rhi/push_constant_info.hpp>

#include <vector>

namespace liger::rhi {

struct InputAssemblyInfo {
  struct VertexInfo {
    struct Attribute {
      Format   format   {Format::kInvalid};
      uint32_t location {0};
      uint32_t offset   {0};
    };

    struct Binding {
      uint32_t binding {0};
      uint32_t stride  {0};

      std::vector<Attribute> attributes;
    };

    std::vector<Binding> bindings;
  };

  enum class Topology : uint8_t {
    /** Specifies a series of separate points. */
    kPointList,

    /** Specifies a series of separate lines. */
    kLineList,

    /** Specifies a series of connected lines, with consecutive ones sharing a vertex. */
    kLineStrip,

    /** Specifies a series of separate triangles. */
    kTriangleList,

    /** Specifies a series of connected triangles, with consecutive ones sharing an edge. */
    kTriangleStrip,

    /** Specifies a series of connected triangles, with all ones sharing a common vertex. */
    kTriangleFan
  };

  VertexInfo vertex_info {};
  Topology   topology    {Topology::kTriangleList};
};

struct RasterizationInfo {
  enum class CullMode : uint8_t {
    kNone,
    kFrontOnly,
    kBackOnly,
    kFrontAndBack
  };

  enum class FrontFace : uint8_t {
    kCounterClockwise,
    kClockwise
  };

  enum class PolygonMode : uint8_t {
    /** Fill the polygon. */
    kFill,

    /** Only render edges of the polygon. */
    kLine
  };

  CullMode    cull_mode    {CullMode::kNone};
  FrontFace   front_face   {FrontFace::kCounterClockwise};
  PolygonMode polygon_mode {PolygonMode::kFill};
};

struct DepthStencilTestInfo {
  enum class CompareOperation : uint8_t {
    kNever,
    kLess,
    kEqual,
    kLessOrEqual,
    kGreater,
    kNotEqual,
    kGreaterOrEqual,
    kAlways
  };

  bool             depth_test_enable       {true};
  bool             depth_write_enable      {true};
  CompareOperation depth_compare_operation {CompareOperation::kNever};

  // TODO(tralf-strues): Stencil test
};

struct ColorBlendInfo {
  /**
   * @brief Specifies blending factor.
   * 
   * Let
   * 1. R_src, G_src, B_src, A_src - source color components
   * 2. R_dst, G_dst, B_dst, A_dst - destination color components
   * 
   * Then factors are defined as follows:
   *
   * Factor            | RGB blend factors                 | Alpha blend factor             |
   * -----------------:|:---------------------------------:|:------------------------------:|
   * kZero             | (0, 0, 0)                         | 0                              |
   * kOne              | (1, 1, 1)                         | 1                              |
   * -----------------:|:---------------------------------:|:------------------------------:|
   * kSrcColor         | (R_src, G_src, B_src)             | A_src                          |
   * kOneMinusSrcColor | (1 - R_src, 1 - G_src, 1 - B_src) | 1 - A_src                      |
   * kDstColor         | (R_dst, G_dst, B_dst)             | A_dst                          |
   * kOneMinusDstColor | (1 - R_dst, 1 - G_dst, 1 - B_dst) | 1 - A_dst                      |
   * -----------------:|:---------------------------------:|:------------------------------:|
   * kSrcAlpha         | (A_src, A_src, A_src)             | A_src                          |
   * kOneMinusSrcAlpha | (1 - A_src, 1 - A_src, 1 - A_src) | 1 - A_src                      |
   * kDstAlpha         | (A_dst, A_dst, A_dst)             | A_dst                          |
   * kOneMinusDstAlpha | (1 - A_dst, 1 - A_dst, 1 - A_dst) | 1 - A_dst                      |
   */
  enum class Factor : uint8_t {
    kZero,
    kOne,

    kSrcColor,
    kOneMinusSrcColor,
    kDstColor,
    kOneMinusDstColor,

    kSrcAlpha,
    kOneMinusSrcAlpha,
    kDstAlpha,
    kOneMinusDstAlpha
  };

  /**
   * @brief Specifies blending operation.
   * 
   * Let
   * 1. R_src, G_src, B_src, A_src - source color components
   * 2. R_dst, G_dst, B_dst, A_dst - destination color components
   * 3. SF_r, SF_g, SF_b, SF_a - source blend factor components
   * 4. DF_r, DF_g, DF_b, DF_a - destination blend factor components
   * 
   * Then operations are defined as follows:
   * 
   * Operation        | Final R/G/B                    | Final A                        |
   * ----------------:|:------------------------------:|:------------------------------:|
   * kAdd             | R_src * SF_r + R_dst * DF_r    | A_src * SF_a + A_dst * DF_a    |
   * kSubtract        | R_src * SF_r - R_dst * DF_r    | A_src * SF_a - A_dst * DF_a    |
   * kReverseSubtract | R_dst * DF_r - R_src * SF_r    | A_dst * DF_a - A_src * SF_a    |
   * kMin             | min(R_src, R_dst)              | min(A_src, A_dst)              |
   * kMax             | max(R_src, R_dst)              | max(A_src, A_dst)              |
   */
  enum class Operation : uint8_t {
    kAdd,
    kSubtract,
    kReverseSubtract,
    kMin,
    kMax
  };

  bool      enable           {true};

  Factor    src_color_factor {Factor::kZero};
  Factor    dst_color_factor {Factor::kZero};
  Operation color_operation  {Operation::kAdd};

  Factor    src_alpha_factor {Factor::kZero};
  Factor    dst_alpha_factor {Factor::kZero};
  Operation alpha_operation  {Operation::kAdd};
};

struct AttachmentInfo {
  std::span<const Format> render_target_formats;
  Format                  depth_stencil_format{Format::kInvalid};
  uint8_t                 samples{1};
};

class IShaderModule;

class IGraphicsPipeline {
 public:
  struct Info {
    InputAssemblyInfo               input_assembly;
    RasterizationInfo               rasterization;
    DepthStencilTestInfo            depth_stencil_test;
    ColorBlendInfo                  blend;
    PushConstantInfo                push_constant;
    AttachmentInfo                  attachments;
    std::span<const IShaderModule*> shader_modules;
  };

  virtual ~IGraphicsPipeline() = 0;
};

}  // namespace liger::rhi