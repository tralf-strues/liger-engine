/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Pipeline.hpp
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

#include <Liger-Engine/RHI/Format.hpp>
#include <Liger-Engine/RHI/PushConstantInfo.hpp>

#include <vector>

namespace liger::rhi {

struct InputAssemblyInfo {
  struct VertexInfo {
    struct Attribute {
      Format   format   {Format::Invalid};
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
    PointList,

    /** Specifies a series of separate lines. */
    LineList,

    /** Specifies a series of connected lines, with consecutive ones sharing a vertex. */
    LineStrip,

    /** Specifies a series of separate triangles. */
    TriangleList,

    /** Specifies a series of connected triangles, with consecutive ones sharing an edge. */
    TriangleStrip,

    /** Specifies a series of connected triangles, with all ones sharing a common vertex. */
    TriangleFan
  };

  VertexInfo vertex_info {};
  Topology   topology    {Topology::TriangleList};
};

struct RasterizationInfo {
  enum class CullMode : uint8_t {
    None,
    FrontOnly,
    BackOnly,
    FrontAndBack
  };

  enum class FrontFace : uint8_t {
    CounterClockwise,
    Clockwise
  };

  enum class PolygonMode : uint8_t {
    /** Fill the polygon. */
    Fill,

    /** Only render edges of the polygon. */
    Line
  };

  CullMode    cull_mode    {CullMode::BackOnly};
  FrontFace   front_face   {FrontFace::CounterClockwise};
  PolygonMode polygon_mode {PolygonMode::Fill};
};

struct DepthStencilTestInfo {
  enum class CompareOperation : uint8_t {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
  };

  bool             depth_test_enable       {true};
  bool             depth_write_enable      {true};
  CompareOperation depth_compare_operation {CompareOperation::Less};

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
    Zero,
    One,

    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,

    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha
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
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
  };

  bool      enable           {true};

  Factor    src_color_factor {Factor::SrcAlpha};
  Factor    dst_color_factor {Factor::DstAlpha};
  Operation color_operation  {Operation::Add};

  Factor    src_alpha_factor {Factor::One};
  Factor    dst_alpha_factor {Factor::Zero};
  Operation alpha_operation  {Operation::Add};
};

struct AttachmentInfo {
  std::span<const Format> render_target_formats;
  Format                  depth_stencil_format{Format::Invalid};
  uint8_t                 samples{1};
};

class IShaderModule;

class IPipeline {
 public:
  struct GraphicsInfo {
    InputAssemblyInfo               input_assembly;
    RasterizationInfo               rasterization;
    DepthStencilTestInfo            depth_stencil_test;
    ColorBlendInfo                  blend;
    PushConstantInfo                push_constant;
    AttachmentInfo                  attachments;
    std::span<const IShaderModule*> shader_modules;
    std::string                     name;
  };

  struct ComputeInfo {
    PushConstantInfo push_constant;
    IShaderModule*   shader_module;
    std::string      name;
  };

  virtual ~IPipeline() = default;
};

}  // namespace liger::rhi