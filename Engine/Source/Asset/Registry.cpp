/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Registry.cpp
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

#include <Liger-Engine/Asset/Registry.hpp>

#include <Liger-Engine/Asset/LogChannel.hpp>

#include <fmt/ostream.h>
#include <fmt/xchar.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>

namespace liger::asset {

namespace fs = std::filesystem;

Registry::Registry(fs::path registry_file)
    : registry_file_(std::move(registry_file)), asset_folder_(registry_file_.parent_path()) {
  if (!ReadRegistryFile()) {
    valid_ = false;
    return;
  }

  for (const auto& [filepath, id] : ids_) {
    LIGER_LOG_INFO(kLogChannelAsset, "AssetEntry [file='{0}', id=0x{1:X}]", filepath.string(), id.Value());
  }

  valid_ = true;
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
  std::ofstream out_file(registry_file_.string(), std::ios::out);
  if (!out_file.is_open()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Couldn't open registry file {0} for save", registry_file_.string());
    return false;
  }

  for (const auto& [id, file_path] : files_) {
    fmt::println(out_file, "- file: {0}", file_path.string());
    fmt::println(out_file, "  id: 0x{0:X}", id.Value());
  }

  return true;
}

const std::filesystem::path& Registry::GetAssetFolder() const {
  return asset_folder_;
}

bool Registry::Contains(Id id) const {
  return files_.contains(id);
}

bool Registry::Contains(const std::filesystem::path& file) const {
  return ids_.contains(file);
}

const fs::path& Registry::GetRelativeFile(Id id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = 0x{0:X})", id.Value());

  return it->second;
}

fs::path Registry::GetAbsoluteFile(Id id) const {
  auto it = files_.find(id);
  LIGER_ASSERT(it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = 0x{0:X})", id.Value());

  return asset_folder_ / it->second;
}

Id Registry::GetId(const std::filesystem::path& file) const {
  auto it = ids_.find(file);
  LIGER_ASSERT(it != ids_.end(), kLogChannelAsset, "Trying to access invalid asset (file = '{0}')", file.string());

  return it->second;
}

inline void ReplaceOcurrences(std::string& str, std::string_view find, std::string_view replace) {
  size_t pos = str.find(find);
  while (pos != std::string::npos) {
    str.replace(pos, find.size(), replace);
    pos = str.find(find, pos + replace.size());
  }
}

Id Registry::Register(const fs::path& file) {
  std::string str_file = file.string();

  ReplaceOcurrences(str_file, "\\\\", "/");
  ReplaceOcurrences(str_file, "\\", "/");

  auto new_file = fs::path(str_file);

  auto new_id = Id::Generate();
  ids_[new_file] = new_id;
  files_[new_id] = std::move(new_file);

  return new_id;
}

void Registry::UpdateFile(Id id, fs::path new_file) {
  auto file_it = files_.find(id);
  LIGER_ASSERT(file_it != files_.end(), kLogChannelAsset, "Trying to access invalid asset (id = 0x{0:X})", id.Value());

  ids_.erase(file_it->second);

  ids_[new_file] = id;
  file_it->second = std::move(new_file);
}

void Registry::Unregister(Id id) {
  auto file_it = files_.find(id);
  if (file_it == files_.end()) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Trying to unregister a non-registered asset (id = 0x{0:X})", id.Value());
    return;
  }

  auto id_it = ids_.find(file_it->second);
  ids_.erase(id_it);
  files_.erase(file_it);
}

bool Registry::ReadRegistryFile() {
  YAML::Node registry = YAML::LoadFile(registry_file_.string());

  if (!registry) {
    LIGER_LOG_ERROR(kLogChannelAsset, "Couldn't open asset registry file \"{}\"", registry_file_.string());
    return false;
  }

  for (auto asset : registry) {
    const auto parse_node = [&asset]<typename T>(std::string_view property, T& out) -> bool {
      auto node = asset[property.data()];
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

    auto file = std::filesystem::path(file_rel);

    // Parse asset id
    auto id = kInvalidId.Value();
    if (!parse_node("id", id)) {
      return false;
    }

    const Id asset_id(id);

    // Add asset
    if (files_.contains(asset_id)) {
      LIGER_LOG_ERROR(kLogChannelAsset, "Duplicate asset id found (id = 0x{0:X})", asset_id.Value());
      return false;
    }

    ids_.emplace(file, asset_id);
    files_.emplace(asset_id, std::move(file));
  }

  return true;
}

}  // namespace liger::asset