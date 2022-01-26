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
#include <chrono>

#include "accelerator_library_interface.hpp"
#include "module_selection.hpp"
#include "pr_module_data.hpp"
#include "scheduled_module.hpp"
#include "scheduling_data.hpp"

using orkhestrafs::core_interfaces::hw_library::OperationPRModules;
using orkhestrafs::dbmstodspi::AcceleratorLibraryInterface;
using orkhestrafs::dbmstodspi::ModuleSelection;
using orkhestrafs::dbmstodspi::ScheduledModule;
using orkhestrafs::dbmstodspi::scheduling_data::ExecutionPlanSchedulingData;

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Class to preprocess given nodes before scheduling.
 */
class ElasticSchedulingGraphParser {
 public:
  static void PreprocessNodes(
      std::vector<std::string>& available_nodes,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::vector<std::string>& processed_nodes,
      std::map<std::string, SchedulingQueryNode>& graph,
      std::map<std::string, TableMetadata>& data_tables,
      AcceleratorLibraryInterface& accelerator_library);

  static void PlaceNodesRecursively(
      std::vector<std::string> available_nodes,
      std::vector<std::string> processed_nodes,
      std::map<std::string, SchedulingQueryNode> graph,
      std::vector<ScheduledModule> current_run,
      std::vector<std::vector<ScheduledModule>> current_plan,
      std::map<std::vector<std::vector<ScheduledModule>>,
               ExecutionPlanSchedulingData>& resulting_plan,
      bool reduce_single_runs,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      int& min_runs, std::map<std::string, TableMetadata> data_tables,
      const std::pair<std::vector<std::vector<ModuleSelection>>,
                      std::vector<std::vector<ModuleSelection>>>& heuristics,
      std::pair<int, int>& statistics_counters,
      const std::vector<std::string>& constrained_first_nodes,
      std::vector<std::string> blocked_nodes,
      std::vector<std::string> next_run_blocked_nodes,
      AcceleratorLibraryInterface& drivers,
      const std::chrono::system_clock::time_point& time_limit,
      bool& trigger_timeout, const bool use_max_runs_cap,
      int streamed_data_size);

 private:
  static auto GetNewStreamedDataSize(
      const std::vector<ScheduledModule>& current_run,
      const std::string& node_name,
      const std::map<std::string, TableMetadata>& data_tables,
      const std::map<std::string, SchedulingQueryNode>& graph) -> int;

  static auto IsTableEqualForGivenNode(
      const std::map<std::string, SchedulingQueryNode>& graph,
      const std::map<std::string, SchedulingQueryNode>& new_graph,
      std::string next_node_name,
      const std::map<std::string, TableMetadata>& data_tables,
      const std::map<std::string, TableMetadata>& new_tables) -> bool;

  static void UpdateNextNodeTables(
      const std::map<std::string, SchedulingQueryNode>& graph,
      std::string node_name,
      std::map<std::string, SchedulingQueryNode>& new_graph,
      std::vector<std::string> skipped_nodes,
      const std::vector<std::string> resulting_tables);

  static auto GetResultingTables(
      const std::vector<std::string>& table_names,
      AcceleratorLibraryInterface& drivers,
      const std::map<std::string, TableMetadata>& tables,
      QueryOperationType operation) -> std::vector<std::string>;

  static void UpdateGraphCapacities(
      const std::map<std::string, SchedulingQueryNode>& graph,
      const std::vector<int>& missing_utility,
      std::map<std::string, SchedulingQueryNode>& new_graph,
      std::string node_name, bool is_node_fully_processed);

  static auto FindMissingUtility(const std::vector<int>& bitstream_capacity,
                                 std::vector<int>& missing_capacity,
                                 const std::vector<int>& node_capacity) -> bool;

  static auto CheckForSkippableSortOperations(
      const std::map<std::string, SchedulingQueryNode>& new_graph,
      const std::map<std::string, TableMetadata>& new_tables,
      std::string node_name,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      AcceleratorLibraryInterface& drivers) -> std::vector<std::string>;

  static auto GetModuleIndex(
      int start_location_index,
      const std::vector<std::pair<int, int>>& taken_positions) -> int;

  static void ReduceSelectionAccordingToHeuristics(
      std::vector<std::pair<int, ScheduledModule>>& resulting_module_placements,
      const std::vector<std::vector<ModuleSelection>>& heuristics);

  static auto GetBitstreamEndFromLibrary(
      int chosen_bitstream_index, int chosen_column_position,
      QueryOperationType current_operation,
      const std::map<QueryOperationType, OperationPRModules>& hw_library)
      -> std::pair<std::string, int>;

  static auto FindAllAvailableBitstreamsAfterMinPos(
      QueryOperationType current_operation, int min_position,
      const std::vector<std::pair<int, int>>& taken_positions,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::vector<std::vector<std::string>>& bitstream_start_locations)
      -> std::vector<std::tuple<int, int, int>>;

  static auto GetChosenModulePlacements(
      std::string node_name,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      std::pair<int, int>& statistics_counters,
      QueryOperationType current_operation,
      const std::vector<std::vector<ModuleSelection>>& heuristics,
      int min_position, const std::vector<std::pair<int, int>>& taken_positions,
      const std::vector<std::vector<std::string>>& bitstream_start_locations)
      -> std::vector<std::pair<int, ScheduledModule>>;

  static auto CurrentRunHasFirstModule(
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::vector<ScheduledModule>& current_run, std::string node_name,
      AcceleratorLibraryInterface& drivers) -> bool;

  static auto RemoveUnavailableNodesInThisRun(
      const std::vector<std::string>& available_nodes,
      const std::vector<ScheduledModule>& current_run,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::map<std::string, SchedulingQueryNode>& graph,
      const std::vector<std::string>& constrained_first_nodes,
      const std::vector<std::string>& blocked_nodes,
      AcceleratorLibraryInterface& drivers) -> std::vector<std::string>;

  static auto GetMinPositionInCurrentRun(
      const std::vector<ScheduledModule>& current_run, std::string node_name,
      const std::map<std::string, SchedulingQueryNode>& graph) -> int;

  static auto GetTakenColumns(const std::vector<ScheduledModule>& current_run)
      -> std::vector<std::pair<int, int>>;

  static auto GetScheduledModulesForNodeAfterPos(
      const std::map<std::string, SchedulingQueryNode>& graph, int min_position,
      std::string node_name,
      const std::vector<std::pair<int, int>>& taken_positions,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::pair<std::vector<std::vector<ModuleSelection>>,
                      std::vector<std::vector<ModuleSelection>>>& heuristics,
      std::pair<int, int>& statistics_counters)
      -> std::vector<std::pair<int, ScheduledModule>>;

  static void FindNextModulePlacement(
      std::map<std::string, SchedulingQueryNode> graph,
      std::map<std::vector<std::vector<ScheduledModule>>,
               ExecutionPlanSchedulingData>& resulting_plan,
      std::vector<std::string> available_nodes,
      std::vector<std::vector<ScheduledModule>> current_plan,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const ScheduledModule& module_placement,
      std::vector<ScheduledModule> current_run, std::string node_name,
      std::vector<std::string> processed_nodes, bool reduce_single_runs,
      int& min_runs, std::map<std::string, TableMetadata> data_tables,
      const std::pair<std::vector<std::vector<ModuleSelection>>,
                      std::vector<std::vector<ModuleSelection>>>& heuristics,
      std::pair<int, int>& statistics_counters,
      const std::vector<std::string>& constrained_first_nodes,
      std::vector<std::string> blocked_nodes,
      std::vector<std::string> next_run_blocked_nodes,
      AcceleratorLibraryInterface& drivers,
      const std::chrono::system_clock::time_point& time_limit,
      bool& trigger_timeout, const bool use_max_runs_cap,
      int streamed_data_size);

  static auto UpdateGraph(
      const std::map<std::string, SchedulingQueryNode>& graph,
      std::string bitstream,
      const std::map<std::string, TableMetadata>& data_tables,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      std::string node_name, const std::vector<int>& capacity,
      QueryOperationType operation, AcceleratorLibraryInterface& drivers)
      -> std::tuple<std::map<std::string, SchedulingQueryNode>,
                    std::map<std::string, TableMetadata>, bool,
                    std::vector<std::string>>;

  static auto CreateNewAvailableNodes(
      const std::map<std::string, SchedulingQueryNode>& graph,
      const std::vector<std::string>& available_nodes,
      const std::vector<std::string>& processed_nodes, std::string node_name,
      bool satisfied_requirements)
      -> std::pair<std::vector<std::string>, std::vector<std::string>>;

  static void UpdateSatisfyingBitstreamsList(
      std::string node_name,
      const std::map<std::string, SchedulingQueryNode>& graph,
      std::map<std::string, SchedulingQueryNode>& new_graph,
      std::vector<std::string>& new_available_nodes,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const std::map<std::string, TableMetadata>& data_tables,
      std::map<std::string, TableMetadata>& new_tables,
      const std::vector<std::string>& new_processed_nodes,
      AcceleratorLibraryInterface& drivers);

  static auto GetNewBlockedNodes(
      const std::vector<std::string>& next_run_blocked_nodes,
      const std::map<QueryOperationType, OperationPRModules>& hw_library,
      const ScheduledModule& module_placement,
      const std::map<std::string, SchedulingQueryNode>& graph,
      AcceleratorLibraryInterface& drivers) -> std::vector<std::string>;

  static void FindDataSensitiveNodeNames(
      const std::string& node_name,
      const std::map<std::string, SchedulingQueryNode>& graph,
      std::vector<std::string>& new_next_run_blocked_nodes,
      AcceleratorLibraryInterface& drivers);
};

}  // namespace orkhestrafs::dbmstodspi