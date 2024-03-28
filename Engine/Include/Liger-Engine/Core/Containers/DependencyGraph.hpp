/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DependencyGraph.hpp
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
#include <cstdint>
#include <vector>

namespace liger {

template <typename Node>
class DAG;

template<>
class DAG<void> {
 public:
  using NodeHandle    = uint32_t;
  using AdjacencyList = std::vector<NodeHandle>;
  using SortedIndex   = uint32_t;
  using SortedList    = std::vector<NodeHandle>;
  using Depth         = uint32_t;
  using DepthList     = std::vector<Depth>;

  DAG() = default;
  explicit DAG(size_t size);

  NodeHandle DeclareNode();

  void AddEdge(NodeHandle from, NodeHandle to);
  bool EdgeExists(NodeHandle from, NodeHandle to) const;

  const AdjacencyList& GetAdjacencyList(NodeHandle handle) const;

  bool TopologicalSort(SortedList& out_sorted);
  bool TopologicalSort(SortedList& out_sorted, DepthList& out_depth, Depth& out_max_depth);

  size_t Size() const;

  DAG<void> Reverse() const;

 private:
  using DFSVisitedList = std::vector<bool>;
  using DFSOnStackList = std::vector<bool>;

  bool TopologicalSortDFS(SortedList& out_sorted, NodeHandle from_handle, DFSVisitedList& visited,
                          DFSOnStackList& on_stack);
  void CalculateDepths(const SortedList& sorted, DepthList& out_depth, Depth& out_max_depth);

  std::vector<AdjacencyList> adj_lists_;
};

template <typename Node>
class DAG : public DAG<void> {
 public:
  using NodeIterator      = typename std::vector<Node>::iterator;
  using ConstNodeIterator = typename std::vector<Node>::const_iterator;
  using DAG<void>::AddEdge;
  using DAG<void>::EdgeExists;
  using DAG<void>::GetAdjacencyList;

  NodeHandle AddNode(Node node);

  template <typename... Args>
  NodeHandle EmplaceNode(Args... args);

  NodeHandle EmplaceNode(Node&& node);

  void AddEdge(const Node& from, const Node& to);

  bool EdgeExists(const Node& from, const Node& to) const;

  Node&       GetNode(NodeHandle handle);
  const Node& GetNode(NodeHandle handle) const;
  NodeHandle  GetNodeHandle(const Node& node) const;

  NodeIterator begin();
  NodeIterator end();
  ConstNodeIterator begin() const;
  ConstNodeIterator end()const;

 private:
  std::vector<Node> nodes_;
};

template <typename Node>
typename DAG<Node>::NodeHandle DAG<Node>::AddNode(Node node) {
  return EmplaceNode(std::move(node));
}

template <typename Node>
template <typename... Args>
typename DAG<Node>::NodeHandle DAG<Node>::EmplaceNode(Args... args) {
  return EmplaceNode(Node{std::forward(args...)});
}

template <typename Node>
typename DAG<Node>::NodeHandle DAG<Node>::EmplaceNode(Node&& node) {
  nodes_.emplace_back(std::move(node));
  return DeclareNode();
}

template <typename Node>
void DAG<Node>::AddEdge(const Node& from, const Node& to) {
  DAG<void>::AddEdge(GetNodeHandle(from), GetNodeHandle(to));
}

template <typename Node>
bool DAG<Node>::EdgeExists(const Node& from, const Node& to) const {
  return DAG<void>::EdgeExists(GetNodeHandle(from), GetNodeHandle(to));
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
typename DAG<Node>::NodeHandle DAG<Node>::GetNodeHandle(const Node& node) const {
  return static_cast<NodeHandle>(&node - nodes_.data());
}

template <typename Node>
typename DAG<Node>::NodeIterator DAG<Node>::begin() {
  return nodes_.begin();
}

template <typename Node>
typename DAG<Node>::NodeIterator DAG<Node>::end() {
  return nodes_.end();
}

template <typename Node>
typename DAG<Node>::ConstNodeIterator DAG<Node>::begin() const {
  return nodes_.cbegin();
}

template <typename Node>
typename DAG<Node>::ConstNodeIterator DAG<Node>::end() const {
  return nodes_.cend();
}

}  // namespace liger