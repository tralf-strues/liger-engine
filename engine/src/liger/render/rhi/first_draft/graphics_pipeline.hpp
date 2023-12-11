/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file graphics_pipeline.hpp
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
#include <liger/render/rhi/shader_module.hpp>

namespace liger::rhi {

/* Pipeline Stages */
enum class PipelineStages : uint32_t {
  kNone                  = 0x0000'0000,
  
  // ...
  
  /** @brief Stage in which indirect draw structures are consumed */
  kDrawIndirect          = 0x0000'0002,

  /** @brief Stage in which vertex and index buffers are consumed */
  kVertexInput           = 0x0000'0004,

  /** @brief Vertex shader stage */
  kVertexShader          = 0x0000'0008,
  
  // ...

  /** 
   * @brief Stage in which depth/stencil tests before the fragment shader are performed. Additionally, in this stage
   *        load operations are performed for framebuffer depth/stencil attachments.
   */
  kEarlyFragmentTests    = 0x0000'0100,

  /** @brief Fragment shader stage */
  kFragmentShader        = 0x0000'0080,

  /**
   * @brief Stage in which depth/stencil tests after the fragment shader are performed. Additionally, in this stage
   *        store operations are performed for framebuffer depth/stencil attachments.
   */
  kLateFragmentTests     = 0x0000'0200,

  /**
   * @brief Stage in which the final color values are output from the pipeline.
   * 
   * This stage is after:
   * 1. Blending final colors
   * 2. Render pass store operations
   * 3. Multisample resolve
   */
  kRenderTargetOutput    = 0x0000'0400,

  /** @brief Compute shader stage */
  kComputeShader         = 0x0000'0800,

  /** @brief Specifies all transfer commands */
  kTransfer              = 0x0000'1000,

  /** @brief Specifies all commands */
  kAllCommands           = 0x0001'0000
};

/* Memory access */
enum class MemoryAccess : uint32_t {
  kNone                = 0x0000'0000,
  kIndirectCommandRead = 0x0000'0001,
  kIndexRead           = 0x0000'0002,
  kVertexAttributeRead = 0x0000'0004,
  kUniformRead         = 0x0000'0008,
  kShaderRead          = 0x0000'0020,
  kShaderWrite         = 0x0000'0040,
  kRenderTargetRead    = 0x0000'0080,
  kRenderTargetWrite   = 0x0000'0100,
  kDepthStencilRead    = 0x0000'0200,
  kDepthStencilWrite   = 0x0000'0400,
  kTransferRead        = 0x0000'0800,
  kTransferWrite       = 0x0000'1000,
};

/* Input Info */
struct InputVertexInfo {
  struct Attribute {
    Format   format   {Format::kInvalid};
    uint32_t location {0};
    uint32_t offset   {0};
  };

  struct Binding {
    uint32_t binding {0};
    uint32_t stride  {0};

    std::vector<Attribute> attribs;
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

struct InputAssemblyInfo {
  Topology topology{Topology::kTriangleList};
};

/* Rasterization Info */
enum class CullMode : uint8_t {
  kNone,
  kFrontOnly,
  kBackOnly,
  kFrontAndBack
};

enum class FrontFace : uint8_t {
  kClockwise,
  kCounterClockwise
};

enum class PolygonMode : uint8_t {
  /** Fill the polygon. */
  kFill,

  /** Only render edges of the polygon. */
  kLine
};

struct RasterizationInfo {
  CullMode    cull_mode    {CullMode::kNone};
  FrontFace   front_face   {FrontFace::kCounterClockwise};
  PolygonMode polygon_mode {PolygonMode::kFill};
};

/* Depth and stencil testing */
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

struct DepthTestInfo {
  bool             test_enable       {true};
  bool             write_enable      {true};
  CompareOperation compare_operation {CompareOperation::kNever};
};

struct StencilTestInfo {
  /* TODO: */
};

/* Color attachment blending */
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
enum class ColorBlendFactor : uint8_t {
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
enum class ColorBlendOperation : uint8_t {
  kAdd,
  kSubtract,
  kReverseSubtract,
  kMin,
  kMax
};

struct BlendInfo {
  bool enable{true};

  ColorBlendFactor    src_color_blend_factor {ColorBlendFactor::kZero};
  ColorBlendFactor    dst_color_blend_factor {ColorBlendFactor::kZero};
  ColorBlendOperation color_blend_operation  {ColorBlendOperation::kAdd};

  ColorBlendFactor    src_alpha_blend_factor {ColorBlendFactor::kZero};
  ColorBlendFactor    dst_alpha_blend_factor {ColorBlendFactor::kZero};
  ColorBlendOperation alpha_blend_operation  {ColorBlendOperation::kAdd};
};

/* Push constant */
struct PushConstantInfo {
  uint32_t size{0};
};

/* Graphics Pipeline */
class GraphicsPipeline final : DeviceResource {
 public:
  static constexpr uint32_t kMaxShaderModules = 6;

  struct Info {
    InputVertexInfo     input_vertex;
    InputAssemblyInfo   input_assembly;

    RasterizationInfo   rasterization;
    DepthTestInfo       depth_test;
    StencilTestInfo     stencil_test;
    BlendInfo           blend;

    PushConstantInfo    push_constant;

    std::array<ShaderModule, kMaxShaderModules> shader_modules;
  };

 public:
  explicit GraphicsPipeline(DeviceResource::InternalHandle internal = nullptr);
  ~GraphicsPipeline() override = default;  
};

GraphicsPipeline::GraphicsPipeline(DeviceResource::InternalHandle internal) : DeviceResource(internal) {}

}  // namespace liger::rhi