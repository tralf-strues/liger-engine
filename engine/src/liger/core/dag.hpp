/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file dag.hpp
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

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <vector>

namespace liger {

template <typename Node>
class DAG {
 public:
  using NodeHandle    = uint32_t;
  using AdjacencyList = std::vector<NodeHandle>;
  using SortedIndex   = uint32_t;
  using SortedList    = std::vector<NodeHandle>;
  using Depth         = uint32_t;
  using DepthList     = std::vector<Depth>;
  using NodeIterator  = std::vector<Node>::iterator;

  NodeHandle AddNode(const Node& node);

  template <typename... Args>
  NodeHandle EmplaceNode(Args... args);

  NodeHandle EmplaceNode(Node&& node);

  void AddEdge(NodeHandle from, NodeHandle to);
  void AddEdge(const Node& from, const Node& to);

  Node&                GetNode(NodeHandle handle);
  const Node&          GetNode(NodeHandle handle) const;
  const AdjacencyList& GetAdjacencyList(NodeHandle handle) const;

  NodeHandle GetNodeHandle(const Node& node) const;

  bool TopologicalSort(SortedList& out_sorted);
  bool TopologicalSort(SortedList& out_sorted, DepthList& out_depth);

  NodeIterator begin();
  NodeIterator end();

 private:
  using DFSVisitedList = std::vector<bool>;
  using DFSOnStackList = std::vector<bool>;

  bool TopologicalSortDFS(SortedList& out_sorted, NodeHandle from_handle, DFSVisitedList& visited,
                          DFSOnStackList& on_stack);
  void CalculateDepths(const SortedList& sorted, DepthList& out_depth);

  std::vector<Node>          nodes_;
  std::vector<AdjacencyList> adj_lists_;
};

template <typename Node>
DAG<Node>::NodeHandle DAG<Node>::AddNode(const Node& node) {
  return EmplaceNode(Node{node});
}

template <typename Node>
template <typename... Args>
DAG<Node>::NodeHandle DAG<Node>::EmplaceNode(Args... args) {
  return EmplaceNode(Node{std::forward(args...)});
}

template <typename Node>
DAG<Node>::NodeHandle DAG<Node>::EmplaceNode(Node&& node) {
  auto handle = static_cast<NodeHandle>(nodes_.size());
  nodes_.emplace_back(std::move(node));
  adj_lists_.resize(nodes_.size());
  return handle;
}

template <typename Node>
void DAG<Node>::AddEdge(NodeHandle from, NodeHandle to) {
  adj_lists_[from].push_back(to);
}

template <typename Node>
void DAG<Node>::AddEdge(const Node& from, const Node& to) {
  AddEdge(GetNodeHandle(from), GetNodeHandle(to));
}

template <typename Node>
Node& DAG<Node>::GetNode(NodeHandle handle) {
  return nodes_[handle];
}

template <typename Node>
const Node& DAG<Node>::GetNode(NodeHandle handle) const {
  return nodes_[handle];
}

template <typename Node>
const DAG<Node>::AdjacencyList& DAG<Node>::GetAdjacencyList(NodeHandle handle) const {
  assert(handle < adj_lists_.size());
  return adj_lists_[handle];
}

template <typename Node>
DAG<Node>::NodeHandle DAG<Node>::GetNodeHandle(const Node& node) const {
  return static_cast<NodeHandle>(&node - nodes_.data());
}

template <typename Node>
bool DAG<Node>::TopologicalSort(SortedList& out_sorted) {
  std::vector<bool> visited(nodes_.size(), false);
  std::vector<bool> on_stack(nodes_.size(), false);

  out_sorted.clear();

  for (NodeHandle from_handle = 0; from_handle < static_cast<NodeHandle>(nodes_.size()); ++from_handle) {
    if (visited[from_handle]) {
      continue;
    }

    if (!TopologicalSortDFS(out_sorted, from_handle, visited, on_stack)) {
      return false;
    }
  }

  std::reverse(out_sorted.begin(), out_sorted.end());
  return true;
}

template <typename Node>
bool DAG<Node>::TopologicalSort(SortedList& out_sorted, DepthList& out_depth) {
  if (!TopologicalSort(out_sorted)) {
    return false;
  }

  CalculateDepths(out_sorted, out_depth);

  return true;
}

template <typename Node>
bool DAG<Node>::TopologicalSortDFS(SortedList& out_sorted, NodeHandle from_handle, DFSVisitedList& visited,
                                   DFSOnStackList& on_stack) {
  visited[from_handle]  = true;
  on_stack[from_handle] = true;

  for (NodeHandle to_handle : adj_lists_[from_handle]) {
    if (visited[to_handle] && on_stack[to_handle]) {
      return false;
    }

    if (visited[to_handle]) {
      continue;
    }

    if (!TopologicalSortDFS(out_sorted, to_handle, visited, on_stack)) {
      return false;
    }
  }

  on_stack[from_handle] = false;
  out_sorted.push_back(from_handle);

  return true;
}

template <typename Node>
void DAG<Node>::CalculateDepths(const SortedList& sorted, DepthList& out_depth) {
  out_depth.resize(nodes_.size());
  std::fill(out_depth.begin(), out_depth.end(), 0);

  for (auto from_handle : sorted) {
    for (auto to_handle : adj_lists_[from_handle]) {
      out_depth[to_handle] = std::max(out_depth[to_handle], out_depth[from_handle] + 1);
    }
  }
}

template <typename Node>
DAG<Node>::NodeIterator DAG<Node>::begin() {
  return nodes_.begin();
}

template <typename Node>
DAG<Node>::NodeIterator DAG<Node>::end() {
  return nodes_.end();
}

}  // namespace liger