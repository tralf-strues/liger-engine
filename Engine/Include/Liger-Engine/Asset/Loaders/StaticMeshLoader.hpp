/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file StaticMeshLoader.hpp
 * @date 2024-05-13
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

#include <Liger-Engine/Asset/Loader.hpp>

namespace liger::rhi {
class IDevice;
}  // namespace liger::rhi

namespace liger::asset::loaders {

/**
 * @brief Static mesh asset loader.
 *
 * File extension: .lsmesh
 *
 * File format (binary):
 * @code{.unparsed}
 *     uint32_t submeshes_count
 *     -------- Submesh 0 --------
 *     uint32_t         vertex_count
 *     uint32_t         index_count
 *     render::Vertex3D vertices[vertex_count]
 *     uint32_t         indices[index_count]
 *     glm::vec4        bounding_sphere
 *     asset::Id        material
 *     -------- Submesh 1 --------
 *     uint32_t         vertex_count
 *     uint32_t         index_count
 *     render::Vertex3D vertices[vertex_count]
 *     uint32_t         indices[index_count]
 *     glm::vec4        bounding_sphere
 *     asset::Id        material
 *     --------    ...    --------
 * @endcode
 */
class StaticMeshLoader : public asset::ILoader {
 public:
  explicit StaticMeshLoader(rhi::IDevice& device);
  ~StaticMeshLoader() override = default;

  std::span<const std::filesystem::path> FileExtensions() const override;

  void Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) override;

 private:
  rhi::IDevice& device_;
};

}  // namespace liger::asset::loaders