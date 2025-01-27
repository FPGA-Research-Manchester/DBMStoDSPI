/*
Copyright 2021 University of Manchester

Licensed under the Apache License, Version 2.0(the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http:  // www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "query_scheduling_helper.hpp"

#include <algorithm>
#include <stdexcept>

using orkhestrafs::dbmstodspi::QuerySchedulingHelper;

auto QuerySchedulingHelper::FindNodePtrIndex(QueryNode* current_node,
                                             QueryNode* previous_node) -> int {
  int index = -1;
  int counter = 0;
  for (const auto& potential_current_node : previous_node->next_nodes) {
    if (potential_current_node != nullptr &&
        potential_current_node == current_node) {
      if (index != -1) {
        throw std::runtime_error(
            "Currently can't support the same module taking multiple inputs "
            "from another module!");
      }
      index = counter;
    }
    counter++;
  }
  if (index == -1) {
    throw std::runtime_error("No current node found!");
  }
  return index;
}

// Assuming sorted by column is always first and the other parameters are legal.
auto QuerySchedulingHelper::IsTableSorted(const TableMetadata& table_data)
    -> bool {
  return table_data.record_count == 0 ||
         table_data.sorted_status.size() == 4 &&
             table_data.sorted_status.at(2) == table_data.record_count;
}

auto QuerySchedulingHelper::AddNewTableToNextNodes(
    std::unordered_map<std::string, SchedulingQueryNode>& graph,
    const std::string& node_name, const std::vector<std::string>& table_names)
    -> bool {
  for (const auto& next_node_name : graph.at(node_name).after_nodes) {
    if (!next_node_name.empty()) {
      for (const auto& [current_node_index, current_stream_index] :
           GetCurrentNodeIndexesByName(graph, next_node_name, node_name)) {
        // TODO(Kaspar): Currently the same for all of them but should be
        // individual
        if (graph.at(next_node_name).data_tables.at(current_node_index) ==
            table_names.at(current_stream_index)) {
          return false;
        }
        graph.at(next_node_name).data_tables.at(current_node_index) =
            table_names.at(current_stream_index);
      }
    }
  }
  return true;
}

auto QuerySchedulingHelper::GetCurrentNodeIndexesByName(
    const std::unordered_map<std::string, SchedulingQueryNode>& graph,
    const std::string& next_node_name, const std::string& current_node_name)
    -> std::vector<std::pair<int, int>> {
  std::vector<std::pair<int, int>> resulting_indexes;
  for (int potential_current_node_index = 0;
       potential_current_node_index <
       graph.at(next_node_name).before_nodes.size();
       potential_current_node_index++) {
    if (graph.at(next_node_name)
            .before_nodes.at(potential_current_node_index)
            .first == current_node_name) {
      auto stream_index = graph.at(next_node_name)
                              .before_nodes.at(potential_current_node_index)
                              .second;
      resulting_indexes.emplace_back(potential_current_node_index,
                                     stream_index);
    }
  }
  if (resulting_indexes.empty()) {
    throw std::runtime_error(
        "No next nodes found with the expected dependency");
  }
  return resulting_indexes;
}

void QuerySchedulingHelper::SetAllNodesAsProcessedAfterGivenNode(
    const std::string& node_name, std::unordered_set<std::string>& past_nodes,
    const std::unordered_map<std::string, SchedulingQueryNode>& graph,
    std::unordered_set<std::string>& current_available_nodes) {
  // You have past nodes including current node
  // You have new nodes
  // You Find new nodes.
  // All of those nodes you remove from available and then you add to past nodes
  // You save all of those nodes and you keep scheduling them while you still
  // have nodes saved.
  auto currently_ignored_nodes = GetNewAvailableNodesAfterSchedulingGivenNode(
      node_name, past_nodes, graph);
  while (!currently_ignored_nodes.empty()) {
    auto current_node_name = *currently_ignored_nodes.begin();
    currently_ignored_nodes.erase(current_node_name);
    current_available_nodes.erase(current_node_name);
    past_nodes.insert(current_node_name);
    UpdateAvailableNodesAfterSchedulingGivenNode(
        current_node_name, past_nodes, graph, currently_ignored_nodes);
  }
}

auto QuerySchedulingHelper::GetNewAvailableNodesAfterSchedulingGivenNode(
    const std::string& node_name,
    const std::unordered_set<std::string>& past_nodes,
    const std::unordered_map<std::string, SchedulingQueryNode>& graph)
    -> std::unordered_set<std::string> {
  std::unordered_set<std::string> potential_nodes(
      graph.at(node_name).after_nodes.begin(),
      graph.at(node_name).after_nodes.end());
  for (const auto& potential_node_name : graph.at(node_name).after_nodes) {
    if (!potential_node_name.empty()) {
      for (const auto& [previous_node_name, node_index] :
           graph.at(potential_node_name).before_nodes) {
        if (!previous_node_name.empty() &&
            past_nodes.find(previous_node_name) == past_nodes.end()) {
          auto search = potential_nodes.find(potential_node_name);
          if (search != potential_nodes.end()) {
            potential_nodes.erase(search);
          }
        }
      }
    }
  }
  potential_nodes.erase("");
  return potential_nodes;
}

void QuerySchedulingHelper::UpdateAvailableNodesAfterSchedulingGivenNode(
    const std::string& node_name,
    const std::unordered_set<std::string>& past_nodes,
    const std::unordered_map<std::string, SchedulingQueryNode>& graph,
    std::unordered_set<std::string>& current_available_nodes) {
  current_available_nodes.merge(GetNewAvailableNodesAfterSchedulingGivenNode(
      node_name, past_nodes, graph));
}

// TODO(Kaspar): This needs improving to support multi inputs and outputs
void QuerySchedulingHelper::RemoveNodeFromGraph(
    std::unordered_map<std::string, SchedulingQueryNode>& graph,
    const std::string& node_name) {
  // We just support one in and one out currently.
  if (graph.at(node_name).before_nodes.size() != 1 ||
      graph.at(node_name).after_nodes.size() != 1) {
    throw std::runtime_error(
        "Only nodes with one input and one output are supported!");
  }

  auto before_node = graph.at(node_name).before_nodes.front();
  auto after_node = graph.at(node_name).after_nodes.front();
  // TODO(Kaspar): No need for the temp copies.
  if (after_node.empty() && before_node.second == -1) {
    // Do nothing
  } else if (after_node.empty()) {
    if (graph.find(before_node.first) != graph.end()) {
      auto new_after_nodes = graph.at(before_node.first).after_nodes;
      new_after_nodes.at(before_node.second) = after_node;
      graph.at(before_node.first).after_nodes = new_after_nodes;
    }
  } else if (before_node.second == -1) {
    int index =
        GetCurrentNodeIndexesByName(graph, after_node, node_name).front().first;
    auto new_before_nodes = graph.at(after_node).before_nodes;
    new_before_nodes.at(index) = before_node;
    graph.at(after_node).before_nodes = new_before_nodes;
  } else {
    if (graph.find(before_node.first) != graph.end()) {
      auto new_after_nodes = graph.at(before_node.first).after_nodes;
      new_after_nodes.at(before_node.second) = after_node;
      graph.at(before_node.first).after_nodes = new_after_nodes;
    }
    int index =
        GetCurrentNodeIndexesByName(graph, after_node, node_name).front().first;
    auto new_before_nodes = graph.at(after_node).before_nodes;
    new_before_nodes.at(index) = before_node;
    graph.at(after_node).before_nodes = new_before_nodes;
  }

  graph.erase(node_name);
}
