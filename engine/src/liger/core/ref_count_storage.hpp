/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ref_count_storage.hpp
 * @date 2024-03-15
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

#include <liger/core/core_log_channel.hpp>
#include <liger/core/log/default_log.hpp>

#include <atomic>
#include <unordered_map>
#include <vector>

namespace liger {

template <typename Key, typename Value>
class RefCountStorage {
 private:
  struct ControlBlock {
    std::atomic<uint32_t> ref_count{0};
    RefCountStorage&      storage;
    Key                   key;
    Value                 value;
  };

 public:
  class Reference {
   public:
    Reference() = default;
    ~Reference();

    Reference(const Reference& other);
    Reference& operator=(const Reference& other);

    Reference(Reference&& other) noexcept;
    Reference& operator=(Reference&& other) noexcept;

    Value& operator*();
    Value* operator->();

    explicit operator bool() const;

   private:
    explicit Reference(ControlBlock* control_block);

    ControlBlock* block_{nullptr};

    friend class RefCountStorage;
  };

  RefCountStorage() = default;
  ~RefCountStorage();

  RefCountStorage(const RefCountStorage& other) = delete;
  RefCountStorage& operator=(const RefCountStorage& other) = delete;

  RefCountStorage(RefCountStorage&& other) noexcept = default;
  RefCountStorage& operator=(RefCountStorage&& other) noexcept = default;

  template <typename... Args>
  [[nodiscard]] Reference Emplace(Key key, Args... args);

  [[nodiscard]] Reference Emplace(Key key, Value&& value);

  [[nodiscard]] bool Contains(Key key) const;

  [[nodiscard]] Reference Get(Key key);

  void CleanUp();

 private:
  void IncrementRef(ControlBlock* block);
  void DecrementRef(ControlBlock* block);

  // FIXME (tralf-strues): The current memory management is just for prototyping, replace ASAP
  std::unordered_map<Key, ControlBlock*> map_;
  std::vector<ControlBlock*>             delete_list_;
};

/* RefCountStorage defintion */
template <typename Key, typename Value>
RefCountStorage<Key, Value>::~RefCountStorage() {
  for (auto& [key, block] : map_) {
    delete block;
    block = nullptr;
  }

  CleanUp();
}

template <typename Key, typename Value>
template <typename... Args>
RefCountStorage<Key, Value>::Reference RefCountStorage<Key, Value>::Emplace(Key key, Args... args) {
  return Emplace(key, Value(std::forward<Args>(args)...));
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference RefCountStorage<Key, Value>::Emplace(Key key, Value&& value) {
  LIGER_ASSERT(map_.find(key) == map_.end(), kLogChannelCore, "Trying to emplace by key already present in the map");

  auto* block = new ControlBlock{.ref_count = 0, .storage = *this, .key = key, .value{std::move(value)}};

  map_.emplace(key, block);
  return Reference(block);
}

template <typename Key, typename Value>
bool RefCountStorage<Key, Value>::Contains(Key key) const {
  return map_.contains(key);
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference RefCountStorage<Key, Value>::Get(Key key) {
  auto it = map_.find(key);
  if (it == map_.end()) {
    return Reference(nullptr);
  }

  return Reference(it->second);
}

template <typename Key, typename Value>
void RefCountStorage<Key, Value>::CleanUp() {
  for (auto* block : delete_list_) {
    delete block;
  }

  delete_list_.clear();
}

template <typename Key, typename Value>
void RefCountStorage<Key, Value>::IncrementRef(ControlBlock* block) {
  block->ref_count.fetch_add(1);
}

template <typename Key, typename Value>
void RefCountStorage<Key, Value>::DecrementRef(ControlBlock* block) {
  if (block->ref_count.fetch_sub(1) == 1) {
    map_.erase(block->key);
    delete_list_.emplace_back(block);
  }
}

/* RefCountStorage::Reference defintion */
template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference::Reference(ControlBlock* control_block) : block_(control_block) {
  if (block_) {
    block_->storage.IncrementRef(block_);
  }
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference::~Reference() {
  if (block_) {
    block_->storage.DecrementRef(block_);
  }

  block_ = nullptr;
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference::Reference(const Reference& other) : block_(other.block_) {
  if (block_) {
    block_->storage.IncrementRef(block_);
  }
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference& RefCountStorage<Key, Value>::Reference::operator=(const Reference& other) {
  if (this == &other) {
    return *this;
  }

  if (block_) {
    block_->storage.DecrementRef(block_);
  }

  block_ = other.block_;

  if (block_) {
    block_->storage.IncrementRef(block_);
  }

  return *this;
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference::Reference(Reference&& other) noexcept : block_(other.block_) {
  other.block_ = nullptr;
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference& RefCountStorage<Key, Value>::Reference::operator=(Reference&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (block_) {
    block_->storage.DecrementRef(block_);
  }

  block_ = other.block_;
  other.block_ = nullptr;

  return *this;
}

template <typename Key, typename Value>
Value& RefCountStorage<Key, Value>::Reference::operator*() {
  return block_->value;
}

template <typename Key, typename Value>
Value* RefCountStorage<Key, Value>::Reference::operator->() {
  return &block_->value;
}

template <typename Key, typename Value>
RefCountStorage<Key, Value>::Reference::operator bool() const {
  return block_ != nullptr;
}

}  // namespace liger