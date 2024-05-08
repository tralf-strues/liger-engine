/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Storage.hpp
 * @date 2024-03-16
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

#include <Liger-Engine/Asset/Id.hpp>
#include <Liger-Engine/Core/Containers/RefCountStorage.hpp>
#include <Liger-Engine/Core/Containers/TypeMap.hpp>

namespace liger::asset {

enum class State : uint32_t {
  Unloaded = 0,
  Loading,
  Loaded,
  Invalid
};

namespace detail {

template <typename Asset>
struct Holder {
  Holder() = default;

  template <typename... Args>
  explicit Holder(Args... args) : asset(std::forward(args)...) {}

  Asset              asset;
  std::atomic<State> state{State::Unloaded};
};

template <typename Asset>
using TemplateAssetStorage = RefCountStorage<Id, Holder<Asset>>;

}  // namespace detail

/**
 * @brief Ref-counted asset handle with state info.
 */
template <typename Asset>
class Handle {
 public:
  Asset& operator*();
  Asset* operator->();

  explicit operator bool() const;

  State GetState() const;

  void UpdateState(State new_state);

 private:
  explicit Handle(typename detail::TemplateAssetStorage<Asset>::Reference&& reference);

  typename detail::TemplateAssetStorage<Asset>::Reference reference_{nullptr};

  friend class Storage;
};

template <typename Asset>
Handle<Asset>::Handle(typename detail::TemplateAssetStorage<Asset>::Reference&& reference) : reference_(reference) {}

template <typename Asset>
Asset& Handle<Asset>::operator*() {
  return reference_->asset;
}

template <typename Asset>
Asset* Handle<Asset>::operator->() {
  return &reference_->asset;
}

template <typename Asset>
Handle<Asset>::operator bool() const {
  return bool(reference_);
}

template <typename Asset>
State Handle<Asset>::GetState() const {
  return reference_->state.load();
}

template <typename Asset>
void Handle<Asset>::UpdateState(State new_state) {
  reference_->state.store(new_state);
}

/**
 * @brief Multi-type asset storage with ref-counting mechanism.
 */
class Storage {
 public:
  template <typename Asset>
  [[nodiscard]] Handle<Asset> Get(Id asset_id);

  template <typename Asset, typename... Args>
  [[nodiscard]] Handle<Asset> Emplace(Id asset_id, Args... args);

 private:
  TypeMap<detail::TemplateAssetStorage> storage_map_;
};

template <typename Asset>
Handle<Asset> Storage::Get(Id asset_id) {
  return Handle<Asset>(storage_map_.Get<Asset>().Get(asset_id));
}

template <typename Asset, typename... Args>
Handle<Asset> Storage::Emplace(Id asset_id, Args... args) {
  return Handle<Asset>(storage_map_.Get<Asset>().Emplace(asset_id, std::forward(args)...));
}

}  // namespace liger::asset