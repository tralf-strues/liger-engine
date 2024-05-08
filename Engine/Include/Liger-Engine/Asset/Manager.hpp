/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Manager.hpp
 * @date 2024-03-17
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

#include <Liger-Engine/Asset/LoaderLibrary.hpp>
#include <Liger-Engine/Asset/LogChannel.hpp>
#include <Liger-Engine/Asset/Registry.hpp>
#include <Liger-Engine/Asset/Storage.hpp>

#include <taskflow/taskflow.hpp>

#include <mutex>

namespace liger::asset {

class Manager {
 public:
  explicit Manager(tf::Executor& executor, std::filesystem::path registry_file);

  bool Valid() const;

  [[nodiscard]] Registry& GetRegistry();
  [[nodiscard]] const Registry& GetRegistry() const;

  void AddLoader(std::unique_ptr<ILoader> loader);

  template <typename Asset>
  [[nodiscard]] Handle<Asset> GetAsset(Id id);

  template <typename Asset>
  [[nodiscard]] Handle<Asset> GetAsset(const std::filesystem::path& file);

 private:
  tf::Executor& executor_;
  Storage       storage_;
  LoaderLibrary loaders_;
  Registry      registry_;
  std::mutex    mutex_;
};

template <typename Asset>
Handle<Asset> Manager::GetAsset(Id id) {
  LIGER_ASSERT(registry_.Valid(), kLogChannelAsset, "Invalid registry");

  std::unique_lock<std::mutex> lock(mutex_);

  auto handle = storage_.Get<Asset>(id);
  if (handle) {
    return handle;
  }

  auto  filepath  = registry_.GetAbsoluteFile(id);
  auto  extension = filepath.extension();
  auto* loader    = loaders_.TryGet(extension);
  LIGER_ASSERT(loader, kLogChannelAsset, "No loader for extension '{0}' found", extension.c_str());

  handle = storage_.Emplace<Asset>(id);
  handle.UpdateState(State::Loading);

  lock.unlock();

  // executor_.silent_async("Load asset", [=, this]() mutable {
    bool loaded = loader->Load(*this, id, filepath);
    handle.UpdateState(loaded ? State::Loaded : State::Invalid);
  // });

  return handle;
}

template <typename Asset>
Handle<Asset> Manager::GetAsset(const std::filesystem::path& file) {
  LIGER_ASSERT(registry_.Valid(), kLogChannelAsset, "Invalid registry");

  std::unique_lock<std::mutex> lock(mutex_);
  auto id = registry_.GetId(file);
  lock.unlock();

  return GetAsset<Asset>(id);
}

}  // namespace liger::asset