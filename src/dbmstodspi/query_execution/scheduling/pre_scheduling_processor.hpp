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

#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "accelerator_library_interface.hpp"
#include "operation_types.hpp"
#include "pr_module_data.hpp"
#include "scheduling_query_node.hpp"
#include "table_data.hpp"

using orkhestrafs::core_interfaces::hw_library::OperationPRModules;
using orkhestrafs::core_interfaces::operation_types::QueryOperationType;
using orkhestrafs::core_interfaces::table_data::TableMetadata;
using orkhestrafs::dbmstodspi::AcceleratorLibraryInterface;
using orkhestrafs::dbmstodspi::SchedulingQueryNode;

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Class to preprocess the graph to improve performance
 */
class PreSchedulingProcessor {
 private:
  /**
   * @brief Method to get the smallest capacity values from the HW library
   * @param hw_library Map of PR modules available for each operation
   * @return Capacity values for each operation
   */
  static auto GetMinimumCapacityValuesFromHWLibrary(
      const std::map<QueryOperationType, OperationPRModules>& hw_library)
      -> std::unordered_map<QueryOperationType, std::vector<int>>;

  // def get_min_requirements(current_node_name, graph, hw_library, data_tables)
  auto GetMinRequirementsForFullyExecutingNode(
      const std::string& node_name,
      const std::unordered_map<std::string, SchedulingQueryNode>& graph,
      const std::map<std::string, TableMetadata>& data_tables)
      -> std::vector<int>;

  // def find_adequate_bitstreams(min_requirements, operation, hw_library)
  auto FindAdequateBitstreams(
      const std::vector<int>& min_requirements,
      std::unordered_map<std::string, SchedulingQueryNode>& graph,
      const std::string& node_name) -> bool;

  // def
  // get_fitting_bitstream_locations_based_on_list(list_of_fitting_bitstreams,
  // start_locations)
  static auto GetFittingBitstreamLocations(
      const std::unordered_set<std::string>& fitting_bitstreams_names,
      const std::vector<std::vector<std::string>>& start_locations)
      -> std::vector<std::vector<std::string>>;

  // def get_worst_case_fully_processed_tables(input_tables,
  // current_node_decorators, data_tables, min_capacity)
  auto SetWorstCaseProcessedTables(
      const std::vector<std::string>& input_table_names,
      const std::vector<int>& min_capacity,
      std::map<std::string, TableMetadata>& data_tables,
      QueryOperationType operation,
      const std::vector<std::string>& output_table_names) -> bool;

  auto SetWorstCaseNodeCapacity(
      const std::string& node_name,
      std::unordered_map<std::string, SchedulingQueryNode>& graph,
      const std::map<std::string, TableMetadata>& data_tables,
      const std::vector<int>& min_capacity) -> bool;

  const std::map<QueryOperationType, OperationPRModules> hw_library_;
  AcceleratorLibraryInterface& accelerator_library_;
  const std::unordered_map<QueryOperationType, std::vector<int>> min_capacity_;

 public:
  PreSchedulingProcessor(
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      AcceleratorLibraryInterface& accelerator_library)
      : hw_library_{hw_library},
        accelerator_library_{accelerator_library},
        min_capacity_{GetMinimumCapacityValuesFromHWLibrary(hw_library)} {};

  // def add_satisfying_bitstream_locations_to_graph(available_nodes, graph,
  // hw_library, data_tables)
  void AddSatisfyingBitstreamLocationsToGraph(
      std::unordered_map<std::string, SchedulingQueryNode>& graph,
      std::map<std::string, TableMetadata>& data_tables,
      std::unordered_set<std::string>& available_nodes,
      std::unordered_set<std::string>& processed_nodes);

  void UpdateOnlySatisfyingBitstreams(
      const std::string& node_name,
      std::unordered_map<std::string, SchedulingQueryNode>& graph,
      const std::map<std::string, TableMetadata>& data_tables);
};

}  // namespace orkhestrafs::dbmstodspi