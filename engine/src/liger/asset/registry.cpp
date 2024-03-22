/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file registry.cpp
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

#include <liger/asset/registry.hpp>

#include <liger/asset/asset_log_channel.hpp>
#include <liger/core/log/default_log.hpp>

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>

namespace liger::asset {

namespace fs = std::filesystem;

Registry::Registry(fs::path registry_file) : registry_file_(std::move(registry_file)) {
  if (!ReadRegistryFile()) {
    valid_ = false;
    return;
  }

  asset_folder_ = registry_file_.parent_path();
}

Registry::~Registry() {
  if (Valid()) {
    Save();
  }
}

Registry::operator bool() const {
  return Valid();
}

bool Registry::Valid() const {
  return valid_;
}

bool Registry::Save() const {
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
    out << YAML::EndMap;
  }

  out << YAML::EndSeq;

  out_file << out.c_str();

  return true;
}

bool Registry::Contains(Id id) const {
  return files_.contains(id);
}

const fs::path& Registry::GetRelativeFile(Id id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  return it->second;
}

fs::path Registry::GetAbsoluteFile(Id id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  return asset_folder_ / it->second;
}

Id Registry::Register(fs::path file) {
  auto new_id = Id::Generate();
  files_[new_id] = std::move(file);

  return new_id;
}

void Registry::UpdateFile(Id id, fs::path new_file) {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = {0})", id.Value());

  it->second = std::move(new_file);
}

void Registry::Unregister(Id id) {
  auto it = files_.find(id);
  if (it == files_.end()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Trying to unregister a non-registered asset (id = {0})", id.Value());
    return;
  }

  files_.erase(it);
}

bool Registry::ReadRegistryFile() {
  YAML::Node registry = YAML::LoadFile(registry_file_.c_str());

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

    auto file = asset_folder_ / std::filesystem::path(file_rel);

    // Parse asset id
    auto id = kInvalidId.Value();
    if (!parse_node("id", id)) {
      return false;
    }

    const Id asset_id(id);

    // Add asset
    if (files_.contains(asset_id)) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Duplicate asset id found (id = {0})", asset_id.Value());
      return false;
    }

    files_.emplace(asset_id, std::move(file));
  }

  return true;
}

}  // namespace liger::asset