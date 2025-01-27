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
#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "query_scheduling_data.hpp"
#include "scheduled_module.hpp"

using orkhestrafs::core_interfaces::query_scheduling_data::QueryNode;
using orkhestrafs::dbmstodspi::ScheduledModule;

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Class to link different runs to reuse memory
 */
class RunLinker {
 public:
  /**
   * @brief Method to empty the given queue and return a queue with nodes which
   * have links cleaned up and saved to a map.
   * @param query_node_runs_queue Queue containing the plan to be executed
   * @param linked_nodes Map of how tables from one nodes get linked to the next
   * ones
   * @return New queue with updated nodes
   */
  static auto LinkPeripheralNodesFromGivenRuns(
      std::queue<
          std::pair<std::vector<ScheduledModule>, std::vector<QueryNode*>>>
          query_node_runs_queue,
      std::queue<
          std::map<std::string,
                   std::map<int, std::vector<std::pair<std::string, int>>>>>&
          linked_nodes)
      -> std::queue<
          std::pair<std::vector<ScheduledModule>, std::vector<QueryNode*>>>;

 private:
  static void CheckExternalLinks(
      const std::vector<QueryNode*>& current_query_nodes,
      std::queue<
          std::map<std::string,
                   std::map<int, std::vector<std::pair<std::string, int>>>>>&
          linked_nodes,
      std::map<std::string, int>& run_counter);
  static auto ReuseMemory(const QueryNode& source_node,
                          const QueryNode& target_node) -> bool;
  static auto IsNodeMissingFromTheVector(
      const QueryNode* linked_node,
      const std::vector<QueryNode*>& current_query_nodes) -> bool;
  static auto FindPreviousNodeLocation(
      const std::vector<QueryNode*>& previous_nodes,
      const QueryNode* previous_node) -> int;
};

}  // namespace orkhestrafs::dbmstodspi