﻿/*
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

#include "graph_creator.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

using dbmstodspi::input_managing::GraphCreator;
using dbmstodspi::query_managing::query_scheduling_data::kSupportedFunctions;
using dbmstodspi::query_managing::query_scheduling_data::
    NodeOperationParameters;

// Using vector of references now but it will be replaced with shared pointers
// in the future.
auto GraphCreator::MakeGraph(std::string input_def_filename)
    -> std::vector<std::shared_ptr<QueryNode>> {
  auto data = json_reader_->ReadInputDefinition(std::move(input_def_filename));

  using ParamsMap = std::map<std::string, std::vector<std::vector<int>>>;

  std::string input_field = "input";
  std::string output_field = "output";
  std::string operation_field = "operation";
  std::string previous_nodes_field = "previous_nodes";
  std::string next_nodes_field = "next_nodes";
  std::string all_operation_parameters_field = "operation_parameters";
  std::string input_stream_params_field = "input_stream_params";
  std::string output_stream_params = "output_stream_params";
  std::string operation_params_field = "operation_params";

  std::map<std::string, std::shared_ptr<QueryNode>> graph_nodes_map;
  std::map<std::string, std::vector<std::string>> previous_nodes;
  std::map<std::string, std::vector<std::string>> next_nodes;

  for (auto const& [node_name, node_parameters] : data) {
    ParamsMap all_operation_parameters_map =
        std::get<ParamsMap>(node_parameters.at(all_operation_parameters_field));
    NodeOperationParameters all_operation_parameters{
        all_operation_parameters_map.at(input_stream_params_field),
        all_operation_parameters_map.at(output_stream_params),
        all_operation_parameters_map.at(operation_params_field)};

    auto output_filenames =
        std::get<std::vector<std::string>>(node_parameters.at(output_field));
    std::vector<bool> is_checked;
    for (const auto& filename : output_filenames) {
      is_checked.push_back(!filename.empty());
    }

    graph_nodes_map.insert(
        {node_name, std::make_shared<QueryNode>(
                        std::get<std::vector<std::string>>(
                            node_parameters.at(input_field)),
                        output_filenames,
                        kSupportedFunctions.at(std::get<std::string>(
                            node_parameters.at(operation_field))),
                        std::vector<std::shared_ptr<QueryNode>>(),
                        std::vector<std::weak_ptr<QueryNode>>(),
                        all_operation_parameters, node_name, is_checked)});

    auto search_previous = node_parameters.find(previous_nodes_field);
    if (search_previous != node_parameters.end()) {
      previous_nodes.insert({node_name, std::get<std::vector<std::string>>(
                                            search_previous->second)});
    }
    auto search_next = node_parameters.find(next_nodes_field);
    if (search_next != node_parameters.end()) {
      next_nodes.insert(
          {node_name, std::get<std::vector<std::string>>(search_next->second)});
    }
  }

  for (auto& [node_name, node] : graph_nodes_map) {
    auto search_previous = previous_nodes.find(node_name);
    if (search_previous != previous_nodes.end()) {
      if (search_previous->second.size() !=
          node->input_data_definition_files.size()) {
        throw std::runtime_error(
            "Incorrect number of input file definitions found!");
      }
      for (int i = 0; i < search_previous->second.size(); i++) {
        if (!search_previous->second[i].empty()) {
          if (node->input_data_definition_files[i] != "") {
            throw std::runtime_error("Input file not required!");
          }
          node->previous_nodes.push_back(
              graph_nodes_map.at(search_previous->second[i]));
        } else {
          if (node->input_data_definition_files[i] == "") {
            throw std::runtime_error("Input file required!");
          }
          node->previous_nodes.push_back(std::weak_ptr<QueryNode>());
        }
      }
    }
    auto search_next = next_nodes.find(node_name);
    if (search_next != next_nodes.end()) {
      for (auto const& next_node_name : search_next->second) {
        if (!next_node_name.empty()) {
          node->next_nodes.push_back(graph_nodes_map.at(next_node_name));
        } else {
          node->next_nodes.push_back(nullptr);
        }
      }
    }
  }

  std::vector<std::shared_ptr<QueryNode>> leaf_nodes;
  for (auto const& [node_name, node] : graph_nodes_map) {
    if (std::none_of(
            node->previous_nodes.begin(), node->previous_nodes.end(),
            [](const std::weak_ptr<QueryNode>& ptr) { return ptr.lock(); })) {
      leaf_nodes.push_back(node);
    }
  }

  return leaf_nodes;
}