/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file resource_version_registry.hpp
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

#include <variant>
#include <vector>

namespace liger::rhi {

template <typename... ResourceTypes>
class ResourceVersionRegistry {
 public:
  using ResourceVersion = uint32_t;

  static constexpr ResourceVersion kInvalidVersion = 0;

 public:
  template <typename ResourceType>
  [[nodiscard]] ResourceVersion AddResource(ResourceType resource);
  [[nodiscard]] ResourceVersion DeclareResource();

  template <typename ResourceType>
  void UpdateResource(ResourceVersion version, ResourceType resource);

  [[nodiscard]] ResourceVersion NextVersion(ResourceVersion prev_version);

  template <typename ResourceType>
  [[nodiscard]] ResourceType GetResource(ResourceVersion version);

 private:
  using NullResource = std::monostate;
  using Resource     = std::variant<NullResource, ResourceTypes...>;

 private:
  std::vector<Resource> resources_;
  std::vector<uint32_t> version_to_resource_;
};

template <typename... ResourceTypes>
template <typename ResourceType>
ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::AddResource(ResourceType resource) {
  auto version = static_cast<ResourceVersion>(version_to_resource_.size());
  auto index   = resources_.size();
  resources_.emplace_back(resource);
  version_to_resource_.emplace_back(index);
  return version;
}

template <typename... ResourceTypes>
ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::DeclareResource() {
  return AddResource(NullResource{});
}

template <typename... ResourceTypes>
template <typename ResourceType>
void ResourceVersionRegistry<ResourceTypes...>::UpdateResource(ResourceVersion version, ResourceType resource) {
  resources_[version_to_resource_[version]] = resource;
}

template <typename... ResourceTypes>
ResourceVersionRegistry<ResourceTypes...>::ResourceVersion
ResourceVersionRegistry<ResourceTypes...>::NextVersion(ResourceVersion prev_version) {
  auto version = static_cast<ResourceVersion>(version_to_resource_.size());
  version_to_resource_.emplace_back(version_to_resource_[prev_version]);
  return version;
}

template <typename... ResourceTypes>
template <typename ResourceType>
ResourceType ResourceVersionRegistry<ResourceTypes...>::GetResource(ResourceVersion version) {
  return std::get<ResourceType>(resources_[version_to_resource_[version]]);
}

}  // namespace liger::rhi