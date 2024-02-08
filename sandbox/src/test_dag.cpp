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

int main() {
  using namespace liger;

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

//   graph.GetNode(node_handle0).write.push_back(0);
//   graph.GetNode(node_handle1).read.push_back(0);

//   graph.GetNode(node_handle1).write.push_back(1);
//   graph.GetNode(node_handle2).read.push_back(1);

//   graph.GetNode(node_handle0).write.push_back(2);
//   graph.GetNode(node_handle3).read.push_back(2);

//   graph.GetNode(node_handle1).write.push_back(3);
//   graph.GetNode(node_handle4).read.push_back(3);

//   graph.GetNode(node_handle3).write.push_back(4);
//   graph.GetNode(node_handle4).read.push_back(4);

//   graph.GetNode(node_handle4).write.push_back(5);
//   graph.GetNode(node_handle5).read.push_back(5);

//   graph.GetNode(node_handle3).write.push_back(6);
//   graph.GetNode(node_handle6).read.push_back(6);

//   graph.GetNode(node_handle6).write.push_back(7);
//   graph.GetNode(node_handle7).read.push_back(7);

//   graph.GetNode(node_handle6).write.push_back(8);
//   graph.GetNode(node_handle8).read.push_back(8);

  graph.AddEdge(node_handle0, node_handle1);
  graph.AddEdge(node_handle1, node_handle2);
  graph.AddEdge(node_handle0, node_handle3);
  graph.AddEdge(node_handle1, node_handle4);
  graph.AddEdge(node_handle3, node_handle4);
  graph.AddEdge(node_handle4, node_handle5);
  graph.AddEdge(node_handle3, node_handle6);
  graph.AddEdge(node_handle6, node_handle7);
  graph.AddEdge(node_handle6, node_handle8);

  DAG<int>::SortedList sorted;
  DAG<int>::DepthList depth;
  bool acyclic = graph.TopologicalSort(sorted, depth);
  assert(acyclic);

  std::ofstream file;
  file.open("dag.dot");

  file << "digraph DAG {\n";

  for (size_t i = 0; i < sorted.size(); ++i) {
    file << "node_" << sorted[i] << " [label=< Node" << sorted[i] << " <BR/> SortedIdx=" << i
         << " <BR/> Depth=" << depth[sorted[i]] << " >]\n";
  }

  for (auto from : sorted) {
    for (auto to : graph.GetAdjacencyList(from)) {
      file << "node_" << from << "->node_" << to << "\n";
    }
  }

  file << "}\n";

  return 0;
}