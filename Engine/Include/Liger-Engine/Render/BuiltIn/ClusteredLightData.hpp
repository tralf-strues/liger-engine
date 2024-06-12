/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ClusteredLightData.hpp
 * @date 2024-06-03
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
#include <Liger-Engine/RHI/RenderGraph.hpp>
#include <Liger-Engine/RHI/ShaderAlignment.hpp>

namespace liger::render {

struct PointLightInfo {
  glm::vec3 color;
  float     intensity;
  float     radius;
};

struct ClusteredLightData {
  glm::uvec3 clusters_count;
  glm::vec2  cluster_z_params;

  rhi::RenderGraph::ResourceVersion rg_point_lights;
  rhi::RenderGraph::ResourceVersion rg_contributing_light_indices;
  rhi::RenderGraph::ResourceVersion rg_light_clusters;
};

}  // namespace liger::render