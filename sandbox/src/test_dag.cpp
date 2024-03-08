/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file test_dependency_graph.cpp
 * @date 2023-12-16
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
#include <cassert>
#include <fstream>

template <typename Node>
using DAG = liger::DAG<Node>;

int main() {

  DAG<int> graph;

  auto node_handle0 = graph.EmplaceNode(0);
  auto node_handle1 = graph.EmplaceNode(1);
  auto node_handle2 = graph.EmplaceNode(2);
  auto node_handle3 = graph.EmplaceNode(3);
  auto node_handle4 = graph.EmplaceNode(4);
  auto node_handle5 = graph.EmplaceNode(5);
  auto node_handle6 = graph.EmplaceNode(6);
  auto node_handle7 = graph.EmplaceNode(7);
  auto node_handle8 = graph.EmplaceNode(8);
  auto node_handle9 = graph.EmplaceNode(9);
  auto node_handle10 = graph.EmplaceNode(10);

  graph.AddEdge(node_handle0, node_handle1);
  graph.AddEdge(node_handle1, node_handle2);
  graph.AddEdge(node_handle0, node_handle3);
  graph.AddEdge(node_handle1, node_handle4);
  graph.AddEdge(node_handle3, node_handle4);
  graph.AddEdge(node_handle4, node_handle5);
  graph.AddEdge(node_handle3, node_handle6);
  graph.AddEdge(node_handle6, node_handle7);
  graph.AddEdge(node_handle6, node_handle8);

  graph.AddEdge(node_handle9, node_handle10);
  graph.AddEdge(node_handle10, node_handle3);

  DAG<int>::SortedList sorted;
  DAG<int>::DepthList depth;
  DAG<int>::Depth max_depth;
  bool acyclic = graph.TopologicalSort(sorted, depth, max_depth);
  assert(acyclic);

  std::ofstream file;
  file.open("dag.dot");

  file << "digraph DAG {\n";

  for (DAG<int>::Depth d = 0; d <= max_depth; ++d) {
    file << "{\n" << "rank=same;\n";

    for (size_t sort_idx = 0; sort_idx < sorted.size(); ++sort_idx) {
      auto node_handle = sorted[sort_idx];

      if (depth[node_handle] == d) {
        file << "node_" << node_handle << " [label=< Node " << node_handle << " <BR/> Sorted = " << sort_idx
             << " <BR/> Depth = " << d << " >]\n";
      }
    }

    file << "}\n";
  }

  for (auto from : sorted) {
    for (auto to : graph.GetAdjacencyList(from)) {
      file << "node_" << from << "->node_" << to << "\n";
    }
  }

  file << "}\n";

  return 0;
}