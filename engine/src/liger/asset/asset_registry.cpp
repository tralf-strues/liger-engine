/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file asset_registry.cpp
 * @date 2024-03-10
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

#include <liger/asset/asset_registry.hpp>

#include <liger/asset/asset_log_channel.hpp>
#include <liger/core/log/default_log.hpp>

#include <yaml-cpp/yaml.h>

#include <fstream>

namespace liger::asset {

AssetRegistry::AssetRegistry(std::filesystem::path registry_file)
    : asset_folder_(registry_file.parent_path()), registry_file_(std::move(registry_file)) {
  if (!ReadRegistryFile()) {
    valid_ = false;
  }
}

AssetRegistry::~AssetRegistry() {
  if (Valid()) {
    Save();
  }
}

AssetRegistry::operator bool() const {
  return Valid();
}

bool AssetRegistry::Valid() const {
  return valid_;
}

bool AssetRegistry::Save() const {
  std::ofstream out_file(registry_file_.c_str(), std::ios::out);
  if (!out_file.is_open()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Couldn't open registry file {0} for save", registry_file_.c_str());
    return false;
  }

  YAML::Emitter out;

  out << YAML::BeginSeq;

  for (const auto& [id, file_path] : files_) {
    out << YAML::BeginMap;
    out << YAML::Key << "file" << YAML::Value << file_path.lexically_relative(asset_folder_);
    out << YAML::Key << "id" << YAML::Value << YAML::Hex << id.Value();

    const auto& dependencies = dependencies_.find(id)->second;
    if (!dependencies.empty()) {
      out << YAML::Key << "dependencies";
      out << YAML::Value << YAML::BeginSeq;

      for (const auto& dependency_id : dependencies) {
        out << YAML::Value << YAML::Hex << dependency_id.Value();
      }

      out << YAML::EndSeq;
    }

    out << YAML::EndMap;
  }

  out << YAML::EndSeq;

  out_file << out.c_str();

  return true;
}

bool AssetRegistry::Contains(AssetId id) const {
  return dependencies_.contains(id);
}

const std::filesystem::path& AssetRegistry::GetRelativeFile(AssetId id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  return it->second;
}

std::filesystem::path AssetRegistry::GetAbsoluteFile(AssetId id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  return asset_folder_ / it->second;
}

AssetRegistry::AssetId AssetRegistry::Register(std::filesystem::path file) {
  auto new_id = AssetId::Generate();
  files_[new_id] = std::move(file);

  return new_id;
}

void AssetRegistry::UpdateFile(AssetId id, std::filesystem::path new_file) {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  it->second = std::move(new_file);
}

void AssetRegistry::Unregister(AssetId id) {
  auto it = files_.find(id);
  if (it == files_.end()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Trying to unregister a non-registered asset (id = {0})", id.Value());
    return;
  }

  files_.erase(it);
  dependencies_.erase(id);
}

void AssetRegistry::AddAssetDependency(AssetId id, AssetId dependency_id) {
  auto it = dependencies_.find(id);
  LIGER_ASSERT(it != dependencies_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  it->second.emplace(dependency_id);
}

void AssetRegistry::RemoveAssetDependency(AssetId id, AssetId dependency_id) {
  auto it = dependencies_.find(id);
  LIGER_ASSERT(it != dependencies_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  it->second.erase(dependency_id);
}

const std::unordered_set<AssetRegistry::AssetId>& AssetRegistry::GetAssetDependencies(AssetId id) const {
  auto it = dependencies_.find(id);
  LIGER_ASSERT(it != dependencies_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  return it->second;
}

bool AssetRegistry::ReadRegistryFile() {
  YAML::Node registry = YAML::LoadFile(registry_file_);

  if (!registry) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Couldn't open asset registry file \"{}\"", registry_file_.c_str());
    return false;
  }

  for (auto asset : registry) {
    const auto parse_node = [&asset]<typename T>(std::string_view property, T& out) -> bool {
      auto node = asset[property];
      if (!node) {
        LIGER_LOG_ERROR(kLogChannelAsset, "Couldn't find \"{}\" property of an asset", property);
        return false;
      }

      out = node.as<T>();

      return true;
    };

    // Parse file
    std::string file_rel;
    if (!parse_node("file", file_rel)) {
      return false;
    }

    const auto file = asset_folder_ / std::filesystem::path(file_rel);

    // Parse asset id
    auto id = kInvalidAssetId.Value();
    if (!parse_node("id", id)) {
      return false;
    }

    const AssetId asset_id(id);

    // Add asset
    if (files_.contains(asset_id)) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Duplicate asset id found (id = {0})", asset_id.Value());
      return false;
    }

    files_.emplace(asset_id, std::move(file));

    dependencies_[asset_id] = {};

    // Parse dependencies (if any)
    if (auto dependencies = asset["dependencies"]; dependencies) {
      if (!dependencies.IsSequence()) {
        LIGER_LOG_ERROR(kLogChannelAsset, "Found \"dependencies\" property which is not a sequence (asset id = {0})",
                        asset_id.Value());
        return false;
      }

      for (auto dependency : dependencies) {
        const auto dependency_id = AssetId(dependency.as<uint64_t>());
        AddAssetDependency(asset_id, dependency_id);
      }
    }
  }

  return true;
}

}  // namespace liger::asset