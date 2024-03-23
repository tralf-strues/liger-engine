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
#include <Liger-Engine/Asset/Registry.hpp>
#include <Liger-Engine/Asset/Storage.hpp>

#include <taskflow/taskflow.hpp>

#include <mutex>

namespace liger::asset {

class Manager {
 public:
  explicit Manager(tf::Executor& executor);

  [[nodiscard]] bool SetRegistry(std::filesystem::path registry_file);

  [[nodiscard]] Registry& GetRegistry();
  [[nodiscard]] const Registry& GetRegistry() const;

  void AddLoader(std::unique_ptr<ILoader> loader);

  template <typename Asset>
  Handle<Asset> GetAsset(Id id);

 private:
  tf::Executor&             executor_;
  Storage                   storage_;
  LoaderLibrary             loaders_;
  std::unique_ptr<Registry> registry_;
  std::mutex                mutex_;
};

}  // namespace liger::asset