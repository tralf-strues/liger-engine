/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file dag.cpp
 * @date 2024-02-24
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

#include <liger/core/dag.hpp>

namespace liger {

DAG<void>::DAG(size_t size) : adj_lists_(size) {}

DAG<void>::NodeHandle DAG<void>::DeclareNode() {
  auto handle = static_cast<NodeHandle>(Size());
  adj_lists_.resize(Size() + 1);
  return handle;
}

void DAG<void>::AddEdge(NodeHandle from, NodeHandle to) {
  adj_lists_[from].push_back(to);
}

bool DAG<void>::EdgeExists(NodeHandle from, NodeHandle to) const {
  for (auto adj_node_handle : adj_lists_[from]) {
    if (adj_node_handle == to) {
      return true;
    }
  }

  return false;
}

const DAG<void>::AdjacencyList& DAG<void>::GetAdjacencyList(NodeHandle handle) const {
  assert(handle < adj_lists_.size());
  return adj_lists_[handle];
}

bool DAG<void>::TopologicalSort(SortedList& out_sorted) {
  std::vector<bool> visited(Size(), false);
  std::vector<bool> on_stack(Size(), false);

  out_sorted.clear();

  for (NodeHandle from_handle = 0; from_handle < static_cast<NodeHandle>(Size()); ++from_handle) {
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

bool DAG<void>::TopologicalSort(SortedList& out_sorted, DepthList& out_depth, Depth& out_max_depth) {
  if (!TopologicalSort(out_sorted)) {
    return false;
  }

  CalculateDepths(out_sorted, out_depth, out_max_depth);

  std::vector<uint32_t> handle_to_sort_idx(out_sorted.size());
  for (uint32_t sort_idx = 0U; sort_idx < out_sorted.size(); ++sort_idx) {
    handle_to_sort_idx[out_sorted[sort_idx]] = sort_idx;
  }

  std::sort(out_sorted.begin(), out_sorted.end(), [&](NodeHandle lhs, NodeHandle rhs) {
    return (out_depth[lhs] < out_depth[rhs]) ||
           (out_depth[lhs] == out_depth[rhs] && handle_to_sort_idx[lhs] < handle_to_sort_idx[rhs]);
  });

  return true;
}

size_t DAG<void>::Size() const {
  return adj_lists_.size();
}

DAG<void> DAG<void>::Reverse() const {
  DAG<void> reverse_dag;
  reverse_dag.adj_lists_.resize(Size());

  for (NodeHandle from = 0; from < Size(); ++from) {
    for (auto to : GetAdjacencyList(from)) {
      reverse_dag.AddEdge(to, from);
    }
  }

  return reverse_dag;
}

bool DAG<void>::TopologicalSortDFS(SortedList& out_sorted, NodeHandle from_handle, DFSVisitedList& visited,
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

void DAG<void>::CalculateDepths(const SortedList& sorted, DepthList& out_depth, Depth& out_max_depth) {
  out_depth.resize(Size());
  std::fill(out_depth.begin(), out_depth.end(), 0);

  for (auto from_handle : sorted) {
    for (auto to_handle : adj_lists_[from_handle]) {
      out_depth[to_handle] = std::max(out_depth[to_handle], out_depth[from_handle] + 1);
      out_max_depth = std::max(out_max_depth, out_depth[to_handle]);
    }
  }
}

}  // namespace liger