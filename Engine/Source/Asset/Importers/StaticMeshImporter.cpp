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

#include <Liger-Engine/Asset/Importers/StaticMeshImporter.hpp>

#include <Liger-Engine/Render/BuiltIn/StaticMeshFeature.hpp>

#include <assimp/GltfMaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <fmt/ostream.h>

namespace liger::asset::importers {

struct MaterialData {
  glm::vec3   base_color;
  glm::vec3   emission_color;
  float       emission_intensity;
  float       metallic;
  float       roughness;

  std::string base_color_map;
  std::string normal_map;
  std::string metallic_roughness_map;
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

bool LoadMaterials(const aiScene* scene, std::vector<MaterialData>& materials,
                   const std::filesystem::path& source_filepath) {
  std::filesystem::path source_dir = source_filepath.parent_path();

  materials.resize(scene->mNumMaterials);
  for (uint32_t material_idx = 0U; material_idx < scene->mNumMaterials; ++material_idx) {
    const aiMaterial* assimp_material = scene->mMaterials[material_idx];

    /* Color values */
    aiColor4D base_color{1, 1, 1, 1};
    if (aiGetMaterialColor(assimp_material, AI_MATKEY_BASE_COLOR, &base_color) == aiReturn_SUCCESS) {
      materials[material_idx].base_color = ConvertAssimpColor(base_color);
    } else if (aiGetMaterialColor(assimp_material, AI_MATKEY_COLOR_DIFFUSE, &base_color) == aiReturn_SUCCESS) {
      materials[material_idx].base_color = ConvertAssimpColor(base_color);
    }

    aiColor4D emission_color{0, 0, 0, 0};
    if (aiGetMaterialColor(assimp_material, AI_MATKEY_COLOR_EMISSIVE, &emission_color) == aiReturn_SUCCESS) {
      materials[material_idx].emission_color = ConvertAssimpColor(emission_color);
    }

    /* Scalars */
    aiGetMaterialFloat(assimp_material, AI_MATKEY_METALLIC_FACTOR,    &materials[material_idx].metallic);
    aiGetMaterialFloat(assimp_material, AI_MATKEY_ROUGHNESS_FACTOR,   &materials[material_idx].roughness);
    aiGetMaterialFloat(assimp_material, AI_MATKEY_EMISSIVE_INTENSITY, &materials[material_idx].emission_intensity);
    
    /* Texture maps */
    aiString base_color_map_path;
    if (assimp_material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &base_color_map_path) == aiReturn_SUCCESS ||
        assimp_material->GetTexture(aiTextureType_DIFFUSE, 0, &base_color_map_path) == aiReturn_SUCCESS) {
      materials[material_idx].base_color_map = (source_dir / base_color_map_path.C_Str()).string();
    }

    aiString normal_map_path;
    if (assimp_material->GetTexture(aiTextureType_NORMALS, 0, &normal_map_path) == aiReturn_SUCCESS) {
      materials[material_idx].normal_map = (source_dir / normal_map_path.C_Str()).string();
    }

    aiString metallic_roughness_map_path;
    if (assimp_material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
                                    &metallic_roughness_map_path) == aiReturn_SUCCESS) {
      materials[material_idx].metallic_roughness_map = (source_dir / metallic_roughness_map_path.C_Str()).string();
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

      vertex.bitangent = glm::vec3{assimp_mesh->mBitangents[vertex_idx].x,
                                   assimp_mesh->mBitangents[vertex_idx].y,
                                   assimp_mesh->mBitangents[vertex_idx].z};
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
  auto base_out_path_textures = dst_folder / "Textures";
  std::filesystem::create_directories(base_out_path_textures);

  std::unordered_map<std::string_view, asset::Id> texture_ids;
  auto save_texture = [&texture_ids, &base_out_path_textures, &registry](std::string_view src_filename) -> asset::Id {
    auto it = texture_ids.find(src_filename);
    if (it != texture_ids.end()) {
      return it->second;
    }

    std::filesystem::path dst_filename = base_out_path_textures / std::filesystem::path(src_filename).filename();
    if (!std::filesystem::copy_file(src_filename, dst_filename)) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Failed to copy texture '{0}' to '{1}'", src_filename, dst_filename.string());
      return asset::kInvalidId;
    }

    auto id = registry.Register(dst_filename.lexically_relative(registry.GetAssetFolder()));
    texture_ids[src_filename] = id;

    return id;
  };

  for (uint32_t material_idx = 0U; material_idx < materials.size(); ++material_idx) {
    if (!materials[material_idx].base_color_map.empty() &&
        save_texture(materials[material_idx].base_color_map) == asset::kInvalidId) {
      return false;
    }

    if (!materials[material_idx].normal_map.empty() &&
        save_texture(materials[material_idx].normal_map) == asset::kInvalidId) {
      return false;
    }

    if (!materials[material_idx].metallic_roughness_map.empty() &&
        save_texture(materials[material_idx].metallic_roughness_map) == asset::kInvalidId) {
      return false;
    }
  }

  auto base_out_path_materials = dst_folder / "Materials";
  std::filesystem::create_directories(base_out_path_materials);

  base_out_path_materials /= base_filename;
  out_ids.reserve(materials.size());

  for (uint32_t material_idx = 0U; material_idx < materials.size(); ++material_idx) {
    std::filesystem::path filename = base_out_path_materials;
    filename += fmt::format("{0}.lmat", material_idx);

    std::ofstream file(filename, std::ios::out);
    if (!file.is_open()) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Failed to open file '{0}'", filename.string());
      return false;
    }

    auto print_vector = [&file](std::string_view name, const glm::vec3& vector) {
      fmt::println(file, "{0}: [{1}, {2}, {3}]", name, vector.x, vector.y, vector.z);
    };

    print_vector("BaseColor", materials[material_idx].base_color);
    print_vector("Emission",  materials[material_idx].emission_color);

    fmt::println(file, "EmissionIntensity: {0}",  materials[material_idx].emission_intensity);
    fmt::println(file, "Metallic: {0}",           materials[material_idx].metallic);
    fmt::println(file, "Roughness: {0}",          materials[material_idx].roughness);

    if (!materials[material_idx].base_color_map.empty()) {
      fmt::println(file, "BaseColorMap: 0x{0:X}", *texture_ids[materials[material_idx].base_color_map]);
    } else {
      fmt::println(file, "BaseColorMap: 0x0");
    }

    if (!materials[material_idx].normal_map.empty()) {
      fmt::println(file, "NormalMap: 0x{0:X}", *texture_ids[materials[material_idx].normal_map]);
    } else {
      fmt::println(file, "NormalMap: 0x0");
    }

    if (!materials[material_idx].metallic_roughness_map.empty()) {
      fmt::println(file, "MetallicRoughnessMap: 0x{0:X}", *texture_ids[materials[material_idx].metallic_roughness_map]);
    } else {
      fmt::println(file, "MetallicRoughnessMap: 0x0");
    }
    
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
  const aiScene* scene = importer.ReadFile(src_str, kProcessFlags);
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
  if (!LoadMaterials(scene, materials, src)) {
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