/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Manager.cpp
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

#include <Liger-Engine/Asset/Manager.hpp>

namespace liger::asset {

Manager::Manager(tf::Executor& executor, std::filesystem::path registry_file)
    : executor_(executor), registry_(std::move(registry_file)) {}

Registry& Manager::GetRegistry() {
  LIGER_ASSERT(registry_.Valid(), kLogChannelAsset, "Invalid registry");
  return registry_;
}

const Registry& Manager::GetRegistry() const {
  LIGER_ASSERT(registry_.Valid(), kLogChannelAsset, "Invalid registry");
  return registry_;
}

void Manager::AddLoader(std::unique_ptr<ILoader> loader) {
  LIGER_ASSERT(registry_.Valid(), kLogChannelAsset, "Invalid registry");
  loaders_.AddLoader(std::move(loader));
}

bool Manager::Valid() const {
  return registry_.Valid();
}

}  // namespace liger::asset