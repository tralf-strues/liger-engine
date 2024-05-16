/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file StaticMeshImporter.cpp
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

#include <Liger-Engine/Asset/Importers/StaticMeshImporter.hpp>

#include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

#include <assimp/GltfMaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <fmt/ostream.h>

namespace liger::asset::importers {

struct MaterialData {
  glm::vec4 color;
};

struct SubmeshData {
  std::vector<render::Vertex3D> vertices;
  std::vector<uint32_t>         indices;
  glm::vec4                     bounding_sphere;
  uint32_t                      material_idx;
};

inline glm::vec4 ConvertAssimpColor(aiColor4D color) {
  return glm::vec4(color.r, color.g, color.b, color.a);
}

inline glm::vec3 ConvertAssimpVec3(aiVector3D vector) {
  return glm::vec3(vector.x, vector.y, vector.z);
}

void CalculateBoundingSphere(SubmeshData& submesh, aiAABB aabb) {
  auto aabb_min = ConvertAssimpVec3(aabb.mMin);
  auto aabb_max = ConvertAssimpVec3(aabb.mMax);

  auto center   = (aabb_max + aabb_min) / 2.0f;
  auto radius   = std::min(glm::length(aabb_max - center), glm::length(aabb_min - center));

  submesh.bounding_sphere = glm::vec4(center, radius);
}

bool LoadMaterials(const aiScene* scene, std::vector<MaterialData>& materials) {
  materials.resize(scene->mNumMaterials);
  for (uint32_t material_idx = 0U; material_idx < scene->mNumMaterials; ++material_idx) {
    /* Color values */
    const aiMaterial* assimp_material = scene->mMaterials[material_idx];

    aiColor4D albedo_color{1, 1, 1, 1};
    if (aiGetMaterialColor(assimp_material, AI_MATKEY_BASE_COLOR, &albedo_color) == aiReturn_SUCCESS) {
      materials[material_idx].color = ConvertAssimpColor(albedo_color);
    } else if (aiGetMaterialColor(assimp_material, AI_MATKEY_COLOR_DIFFUSE, &albedo_color) == aiReturn_SUCCESS) {
      materials[material_idx].color = ConvertAssimpColor(albedo_color);
    }
  }

  return true;
}

bool LoadSubmeshes(const aiScene* scene, std::vector<SubmeshData>& submeshes) {
  submeshes.resize(scene->mNumMeshes);

  for (uint32_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
    const aiMesh* assimp_mesh = scene->mMeshes[mesh_idx];

    const uint32_t vertex_count = assimp_mesh->mNumVertices;
    const uint32_t index_count  = 3 * assimp_mesh->mNumFaces;

    auto& submesh = submeshes[mesh_idx];

    auto& vertices = submesh.vertices;
    vertices.resize(vertex_count);

    auto& indices  = submesh.indices;
    indices.resize(index_count);

    for (uint32_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
      auto& vertex = vertices[vertex_idx];

      vertex.position = glm::vec3{assimp_mesh->mVertices[vertex_idx].x,
                                  assimp_mesh->mVertices[vertex_idx].y,
                                  assimp_mesh->mVertices[vertex_idx].z};

      if (assimp_mesh->mTextureCoords[0]) {
        vertex.tex_coords = glm::vec2{assimp_mesh->mTextureCoords[0][vertex_idx].x,
                                      assimp_mesh->mTextureCoords[0][vertex_idx].y};
      }

      vertex.normal = glm::vec3{assimp_mesh->mNormals[vertex_idx].x,
                                assimp_mesh->mNormals[vertex_idx].y,
                                assimp_mesh->mNormals[vertex_idx].z};

      vertex.tangent = glm::vec3{assimp_mesh->mTangents[vertex_idx].x,
                                 assimp_mesh->mTangents[vertex_idx].y,
                                 assimp_mesh->mTangents[vertex_idx].z};

      //vertex.bitangent =
      //    glm::vec3{mesh->mBitangents[vertex_idx].x, mesh->mBitangents[vertex_idx].y, mesh->mBitangents[vertex_idx].z};
    }

    for (uint32_t i = 0; i < assimp_mesh->mNumFaces; ++i) {
      aiFace face = assimp_mesh->mFaces[i];
      if (face.mNumIndices != 3U) {
        LIGER_ASSERT(face.mNumIndices == 3U, kLogChannelAsset, "Only triangle meshes are supported at the moment.");
        return false;
      }

      for (uint32_t j = 0U; j < face.mNumIndices; ++j) {
        indices[3U * i + j] = face.mIndices[j];
      }
    }

    submesh.material_idx = assimp_mesh->mMaterialIndex;
    CalculateBoundingSphere(submesh, assimp_mesh->mAABB);
  }

  return true;
}

bool SaveMaterials(asset::Registry& registry, const std::filesystem::path& dst_folder,
                   const std::filesystem::path& base_filename, const std::vector<MaterialData>& materials,
                   std::vector<asset::Id>& out_ids) {
  auto base_out_path = dst_folder / "Materials";
  std::filesystem::create_directories(base_out_path);

  base_out_path /= base_filename;
  out_ids.reserve(materials.size());

  for (uint32_t material_idx = 0U; material_idx < materials.size(); ++material_idx) {
    std::filesystem::path filename = base_out_path;
    filename += fmt::format("{0}.lmat", material_idx);

    std::ofstream file(filename, std::ios::out);
    if (!file.is_open()) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filename.string());
      return false;
    }

    fmt::print(file, "Color: {0}", materials[material_idx].color);
    
    file.close();

    out_ids.push_back(registry.Register(filename.lexically_relative(registry.GetAssetFolder())));
  }

  return true;
}

template <typename T>
void BinaryWrite(std::ofstream& os, const T* data, uint32_t count = 1U) {
  os.write(reinterpret_cast<const char*>(data), count * sizeof(T));
}

bool SaveMesh(asset::Registry& registry, const std::filesystem::path& dst_folder,
              const std::filesystem::path& base_filename, const std::vector<SubmeshData>& submeshes,
              const std::vector<asset::Id>& material_asset_ids, asset::Id& out_id) {
  std::filesystem::create_directories(dst_folder);

  auto filename = dst_folder / base_filename;
  filename += ".lsmesh";

  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filename.string());
    return false;
  }

  uint32_t submeshes_count = submeshes.size();
  BinaryWrite(file, &submeshes_count);

  for (const auto& submesh : submeshes) {
    uint32_t vertex_count = submesh.vertices.size();
    uint32_t index_count  = submesh.indices.size();

    BinaryWrite(file, &vertex_count);
    BinaryWrite(file, &index_count);

    BinaryWrite(file, submesh.vertices.data(), vertex_count);
    BinaryWrite(file, submesh.indices.data(), index_count);
    BinaryWrite(file, &submesh.bounding_sphere);
    BinaryWrite(file, &material_asset_ids[submesh.material_idx]);
  }

  file.close();

  out_id = registry.Register(filename.lexically_relative(registry.GetAssetFolder()));

  return true;
}

asset::IImporter::Result StaticMeshImporter::Import(asset::Registry& registry, const std::filesystem::path& src,
                                                    const std::filesystem::path& dst_folder) const {
  constexpr uint32_t kProcessFlags = aiProcess_Triangulate |
                                     aiProcess_SortByPType |
                                     aiProcess_GenSmoothNormals |
                                     aiProcess_CalcTangentSpace |
                                     aiProcess_GenUVCoords |
                                     aiProcess_GenBoundingBoxes;
  const Result kFailedResult{.success = false};

  auto src_str = src.string();

  Assimp::Importer importer;
  const aiScene*   scene = importer.ReadFile(src_str, kProcessFlags);
  if (scene == nullptr) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'.", src_str);
    return kFailedResult;
  }

  if (scene->mRootNode == nullptr) {
    LIGER_LOG_ERROR(kLogChannelAsset, "File '{0}' does not contain a root node.", src_str);
    return kFailedResult;
  }

  if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Scene in '{0}' is incomplete.", src_str);
    return kFailedResult;
  }

  if (scene->mNumMeshes == 0) {
    LIGER_LOG_ERROR(kLogChannelAsset, "No meshes found in '{0}'.", src_str);
    return kFailedResult;
  }

  std::vector<MaterialData> materials;
  if (!LoadMaterials(scene, materials)) {
    return kFailedResult;
  }

  std::vector<SubmeshData> submeshes;
  if (!LoadSubmeshes(scene, submeshes)) {
    return kFailedResult;
  }

  auto base_filename = src.stem();
  auto abs_dst_folder = registry.GetAssetFolder() / dst_folder;

  std::vector<asset::Id> material_asset_ids;
  if (!SaveMaterials(registry, abs_dst_folder, base_filename, materials, material_asset_ids)) {
    return kFailedResult;
  }

  asset::Id mesh_asset_id;
  if (!SaveMesh(registry, abs_dst_folder, base_filename, submeshes, material_asset_ids, mesh_asset_id)) {
    return kFailedResult;
  }

  Result result;
  result.success         = true;
  result.imported_assets = std::move(material_asset_ids);
  result.imported_assets.emplace_back(mesh_asset_id);

  return result;
}

}  // namespace liger::asset::importers