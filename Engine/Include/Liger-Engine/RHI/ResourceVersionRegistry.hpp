/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ResourceVersionRegistry.hpp
 * @date 2024-01-02
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

#include <optional>
#include <variant>
#include <vector>

namespace liger::rhi {

template <typename... ResourceTypes>
class ResourceVersionRegistry {
 public:
  using ResourceVersion = uint32_t;
  using ResourceId      = uint32_t;
  using NullResource    = std::monostate;
  using Resource        = std::variant<NullResource, ResourceTypes...>;
  using iterator        = typename std::vector<Resource>::iterator;
  using const_iterator  = typename std::vector<Resource>::const_iterator;

  static constexpr ResourceVersion kInvalidVersion = 0;

  template <typename ResourceType>
  [[nodiscard]] ResourceVersion AddResource(ResourceType resource);

  template <typename ResourceType>
  [[nodiscard]] ResourceVersion DeclareResource();

  template <typename ResourceType>
  void UpdateResource(ResourceId id, ResourceType resource);

  [[nodiscard]] ResourceVersion NextVersion(ResourceVersion prev_version);

  template <typename ResourceType>
  [[nodiscard]] ResourceType GetResourceByVersion(ResourceVersion version);

  template <typename ResourceType>
  [[nodiscard]] std::optional<ResourceType> TryGetResourceByVersion(ResourceVersion version);

  template <typename ResourceType>
  [[nodiscard]] std::optional<const ResourceType> TryGetResourceByVersion(ResourceVersion version) const;

  template <typename ResourceType>
  [[nodiscard]] ResourceType GetResourceById(ResourceId id);

  template <typename ResourceType>
  [[nodiscard]] std::optional<ResourceType> TryGetResourceById(ResourceId id);

  template <typename ResourceType>
  [[nodiscard]] std::optional<const ResourceType> TryGetResourceById(ResourceId id) const;

  [[nodiscard]] ResourceId GetResourceId(ResourceVersion version) const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  uint32_t GetVersionsCount() const;
  uint32_t GetResourceCount() const;

 private:
  std::vector<Resource>   resources_;
  std::vector<ResourceId> version_to_id_;
};

template <typename... ResourceTypes>
template <typename ResourceType>
typename ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::AddResource(ResourceType resource) {
  auto version = static_cast<ResourceVersion>(version_to_id_.size());
  auto id      = static_cast<ResourceId>(resources_.size());
  resources_.emplace_back(resource);
  version_to_id_.emplace_back(id);
  return version;
}

template <typename... ResourceTypes>
template <typename ResourceType>
typename ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::DeclareResource() {
  return AddResource(ResourceType{});
}

template <typename... ResourceTypes>
template <typename ResourceType>
void ResourceVersionRegistry<ResourceTypes...>::UpdateResource(ResourceId id, ResourceType resource) {
  resources_[id] = resource;
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::NextVersion(ResourceVersion prev_version) {
  auto version = static_cast<ResourceVersion>(version_to_id_.size());
  version_to_id_.emplace_back(version_to_id_[prev_version]);
  return version;
}

template <typename... ResourceTypes>
template <typename ResourceType>
ResourceType ResourceVersionRegistry<ResourceTypes...>::GetResourceByVersion(ResourceVersion version) {
  return GetResourceById<ResourceType>(GetResourceId(version));
}

template <typename... ResourceTypes>
template <typename ResourceType>
std::optional<ResourceType> ResourceVersionRegistry<ResourceTypes...>::TryGetResourceByVersion(ResourceVersion version) {
  return TryGetResourceById<ResourceType>(GetResourceId(version));
}

template <typename... ResourceTypes>
template <typename ResourceType>
std::optional<const ResourceType> ResourceVersionRegistry<ResourceTypes...>::TryGetResourceByVersion(
    ResourceVersion version) const {
  return TryGetResourceById<ResourceType>(GetResourceId(version));
}

template <typename... ResourceTypes>
template <typename ResourceType>
ResourceType ResourceVersionRegistry<ResourceTypes...>::GetResourceById(ResourceId id) {
  return std::get<ResourceType>(resources_[id]);
}

template <typename... ResourceTypes>
template <typename ResourceType>
std::optional<ResourceType> ResourceVersionRegistry<ResourceTypes...>::TryGetResourceById(ResourceId id) {
  auto resource = std::get_if<ResourceType>(&resources_[id]);
  return (resource != nullptr) ? std::optional(*resource) : std::nullopt;
}

template <typename... ResourceTypes>
template <typename ResourceType>
std::optional<const ResourceType> ResourceVersionRegistry<ResourceTypes...>::TryGetResourceById(ResourceId id) const {
  auto resource = std::get_if<ResourceType>(&resources_[id]);
  return (resource != nullptr) ? std::optional(*resource) : std::nullopt;
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::iterator ResourceVersionRegistry<ResourceTypes...>::begin() {
  return resources_.begin();
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::iterator ResourceVersionRegistry<ResourceTypes...>::end() {
  return resources_.end();
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::const_iterator ResourceVersionRegistry<ResourceTypes...>::begin() const {
  return resources_.begin();
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::const_iterator ResourceVersionRegistry<ResourceTypes...>::end() const {
  return resources_.end();
}

template <typename... ResourceTypes>
uint32_t ResourceVersionRegistry<ResourceTypes...>::GetVersionsCount() const {
  return static_cast<uint32_t>(version_to_id_.size());
}

template <typename... ResourceTypes>
uint32_t ResourceVersionRegistry<ResourceTypes...>::GetResourceCount() const {
  return static_cast<uint32_t>(resources_.size());
}

template <typename... ResourceTypes>
typename ResourceVersionRegistry<ResourceTypes...>::ResourceId ResourceVersionRegistry<ResourceTypes...>::GetResourceId(
    ResourceVersion version) const {
  return version_to_id_[version];
}

}  // namespace liger::rhi