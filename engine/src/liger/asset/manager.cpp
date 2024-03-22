/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file manager.cpp
 * @date 2024-03-19
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

#include <liger/asset/manager.hpp>

#include <taskflow/taskflow.hpp>

namespace liger::asset {

Manager::Manager(tf::Executor& executor) : executor_(executor) {}

bool Manager::SetRegistry(std::filesystem::path registry_file) {
  registry_ = std::make_unique<Registry>(std::move(registry_file));
  return registry_->Valid();
}

Registry& Manager::GetRegistry() { return *registry_; }

const Registry& Manager::GetRegistry() const { return *registry_; }

void Manager::AddLoader(std::unique_ptr<ILoader> loader) { loaders_.AddLoader(std::move(loader)); }

template <typename Asset>
Handle<Asset> Manager::GetAsset(Id id) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto handle = storage_.Get<Asset>(id);
  if (handle) {
    return handle;
  }

  auto  filepath  = registry_->GetAbsoluteFile(id);
  auto  extension = filepath.extension();
  auto* loader    = loaders_.TryGet(extension);

  handle = storage_.Emplace(id, Asset{});
  handle.UpdateState(State::kLoading);

  lock.unlock();

  executor_.silent_async("Load Asset", [this, loader, id, filepath, handle]() {
    bool loaded = loader->Load(*this, id, filepath);
    handle.UpdateState(loaded ? State::kLoaded : State::kInvalid);
  });

  return handle;
}

}  // namespace liger::asset