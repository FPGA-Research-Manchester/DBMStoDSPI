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

#include "elastic_resource_scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "elastic_scheduling_graph_parser.hpp"
#include "logger.hpp"
#include "scheduling_data.hpp"
#include "time_limit_execption.hpp"

using orkhestrafs::dbmstodspi::logging::Log;
using orkhestrafs::dbmstodspi::logging::LogLevel;

using orkhestrafs::dbmstodspi::ElasticResourceNodeScheduler;
using orkhestrafs::dbmstodspi::ElasticSchedulingGraphParser;
using orkhestrafs::dbmstodspi::TimeLimitException;

void ElasticResourceNodeScheduler::RemoveUnnecessaryTables(
    const std::unordered_map<std::string, SchedulingQueryNode> &graph,
    std::map<std::string, TableMetadata> &tables) {
  //  std::map<std::string, TableMetadata> resulting_tables;
  //  for (const auto &[table_name, table_data] : tables) {
  //    if (std::any_of(graph.begin(), graph.end(), [&](const auto &p) {
  //          return std::find(p.second.data_tables.begin(),
  //                           p.second.data_tables.end(),
  //                           table_name) != p.second.data_tables.end();
  //        })) {
  //      resulting_tables.insert({table_name, table_data});
  //    }
  //  }
  //  tables = resulting_tables;
}

auto ElasticResourceNodeScheduler::CalculateTimeLimit(
    const std::unordered_map<std::string, SchedulingQueryNode> &graph,
    const std::map<std::string, TableMetadata> &data_tables,
    double config_speed, double streaming_speed,
    const std::unordered_map<QueryOperationType, int> &operation_costs)
    -> double {
  int smallest_config_size = 0;
  for (const auto &[node_name, parameters] : graph) {
    smallest_config_size += operation_costs.at(parameters.operation);
  }
  double config_time = smallest_config_size / config_speed;
  int table_sizes = 0;
  for (const auto &[node_name, parameters] : graph) {
    for (const auto &table_name : parameters.data_tables) {
      if (!table_name.empty()) {
        table_sizes += data_tables.at(table_name).record_count *
                       data_tables.at(table_name).record_size * 4;
      }
    }
  }
  double execution_time = table_sizes / streaming_speed;
  return config_time + execution_time;
}

auto ElasticResourceNodeScheduler::GetLargestModulesSizes(
    const std::map<QueryOperationType, OperationPRModules> & /*hw_libary*/)
    -> std::unordered_map<QueryOperationType, int> {
  // Hardcoded for now.
  std::unordered_map<QueryOperationType, int> operation_costs = {
      {QueryOperationType::kFilter, 315456},
      {QueryOperationType::kLinearSort, 770784},
      {QueryOperationType::kMergeSort, 770784},
      {QueryOperationType::kJoin, 462768},
      {QueryOperationType::kAddition, 315456},
      {QueryOperationType::kMultiplication, 916608},
      {QueryOperationType::kAggregationSum, 229152},
  };
  return operation_costs;
}

auto ElasticResourceNodeScheduler::ScheduleAndGetAllPlans(
    const std::unordered_set<std::string> &starting_nodes,
    const std::unordered_set<std::string> &processed_nodes,
    const std::unordered_map<std::string, SchedulingQueryNode> &graph,
    const std::map<std::string, TableMetadata> &tables, const Config &config)
    -> std::tuple<int,
                  std::map<std::vector<std::vector<ScheduledModule>>,
                           ExecutionPlanSchedulingData>,
                  long long, bool, std::pair<int, int>> {
  double time_limit_duration_in_seconds = config.time_limit_duration_in_seconds;
  if (time_limit_duration_in_seconds == -1) {
    auto operation_costs = GetLargestModulesSizes(config.pr_hw_library);
    time_limit_duration_in_seconds =
        CalculateTimeLimit(graph, tables, config.configuration_speed,
                           config.streaming_speed, operation_costs);
  }
  auto time_limit =
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(int(time_limit_duration_in_seconds * 1000));

  scheduler_->SetTimeLimit(time_limit);
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();

  // Configure the runtime_error behaviour
  try {
    scheduler_->PlaceNodesRecursively(
        std::move(starting_nodes), std::move(processed_nodes), std::move(graph),
        {}, {}, tables, {}, {}, 0);
  } catch (TimeLimitException &e) {
    Log(LogLevel::kInfo, "Timeout of " +
                             std::to_string(time_limit_duration_in_seconds) +
                             " seconds hit by the scheduler.");
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  return {-1, scheduler_->GetResultingPlan(),
          std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
              .count(),
          scheduler_->GetTimeoutStatus(), scheduler_->GetStats()};
}

void ElasticResourceNodeScheduler::BenchmarkScheduling(
    const std::unordered_set<std::string> &first_node_names,
    std::unordered_set<std::string> starting_nodes,
    std::unordered_set<std::string> &processed_nodes,
    std::unordered_map<std::string, SchedulingQueryNode> graph,
    AcceleratorLibraryInterface &drivers,
    std::map<std::string, TableMetadata> tables,
    std::vector<ScheduledModule> &current_configuration, const Config &config,
    std::map<std::string, double> &benchmark_data) {
  Log(LogLevel::kTrace, "Schedule round");
  std::chrono::steady_clock::time_point begin_pre_process =
      std::chrono::steady_clock::now();
  if (!scheduler_) {
    auto heuristic_choices = GetDefaultHeuristics();
    scheduler_ = std::make_unique<ElasticSchedulingGraphParser>(
        config.pr_hw_library, heuristic_choices.at(config.heuristic_choice),
        first_node_names, drivers, config.use_max_runs_cap,
        config.reduce_single_runs);
  }
  // RemoveUnnecessaryTables(graph, tables);
  scheduler_->PreprocessNodes(starting_nodes, processed_nodes, graph, tables);
  std::chrono::steady_clock::time_point end_pre_process =
      std::chrono::steady_clock::now();
  auto pre_process_time = std::chrono::duration_cast<std::chrono::microseconds>(
                              end_pre_process - begin_pre_process)
                              .count();
  auto [min_runs, resulting_plans, scheduling_time, timed_out, stats] =
      ScheduleAndGetAllPlans(starting_nodes, processed_nodes, graph, tables,
                             config);
  std::chrono::steady_clock::time_point begin_cost_eval =
      std::chrono::steady_clock::now();
  // TODO: Make a separate method for non benchmark for performance
  auto [best_plan, new_last_config, data_amount, configuration_amount] =
      plan_evaluator_->GetBestPlan(
          min_runs, current_configuration, config.resource_string,
          config.utilites_scaler, config.config_written_scaler,
          config.utility_per_frame_scaler, resulting_plans,
          config.cost_of_columns, config.streaming_speed,
          config.configuration_speed);
  std::chrono::steady_clock::time_point end_cost_eval =
      std::chrono::steady_clock::now();
  auto cost_eval_time = std::chrono::duration_cast<std::chrono::microseconds>(
                            end_cost_eval - begin_cost_eval)
                            .count();

  auto overall_time = std::chrono::duration_cast<std::chrono::microseconds>(
                          end_cost_eval - begin_pre_process)
                          .count();
  benchmark_data["discarded_placements"] += stats.second;
  benchmark_data["placed_nodes"] += stats.first;
  benchmark_data["plan_count"] += resulting_plans.size();
  std::cout << "plan_count: " << std::to_string(resulting_plans.size())
            << std::endl;
  benchmark_data["pre_process_time"] += pre_process_time;
  // std::cout << "pre_process_time: " << std::to_string(pre_process_time)
  //          << std::endl;
  benchmark_data["schedule_time"] += scheduling_time;
  // std::cout << "schedule_time: " << std::to_string(scheduling_time)
  //          << std::endl;
  benchmark_data["timeout"] += static_cast<double>(timed_out);
  // std::cout << "timeout: " << std::to_string(timed_out) << std::endl;
  benchmark_data["cost_eval_time"] += cost_eval_time;
  // std::cout << "cost_eval_time: " << std::to_string(cost_eval_time)
  //          << std::endl;
  benchmark_data["overall_time"] += overall_time;
  std::cout << "overall_time: " << std::to_string(overall_time) << std::endl;
  benchmark_data["run_count"] += best_plan.size();
  // std::cout << "run_count: " << std::to_string(best_plan.size()) <<
  // std::endl;
  benchmark_data["data_amount"] += data_amount;
  // std::cout << "data_amount: " << std::to_string(data_amount) << std::endl;
  benchmark_data["configuration_amount"] += configuration_amount;
  // std::cout << "configuration_amount: " <<
  // std::to_string(configuration_amount)
  //          << std::endl;
  benchmark_data["schedule_count"] += 1;

  // starting_nodes = resulting_plans.at(best_plan).available_nodes;
  processed_nodes = resulting_plans.at(best_plan).processed_nodes;
  // graph = resulting_plans.at(best_plan).graph;
  // tables = resulting_plans.at(best_plan).tables;

  current_configuration = new_last_config;

  /*for (const auto &run : best_plan) {
    for (const auto &node : run) {
      std::cout << " " << node.node_name << " ";
    }
    std::cout << std::endl;
  }*/
}

auto ElasticResourceNodeScheduler::GetNextSetOfRuns(
    std::vector<QueryNode *> &available_nodes,
    const std::unordered_set<std::string> &first_node_names,
    std::unordered_set<std::string> starting_nodes,
    std::unordered_map<std::string, SchedulingQueryNode> graph,
    AcceleratorLibraryInterface &drivers,
    std::map<std::string, TableMetadata> tables,
    const std::vector<ScheduledModule> &current_configuration,
    const Config &config, std::unordered_set<std::string> &skipped_nodes)
    -> std::queue<
        std::pair<std::vector<ScheduledModule>, std::vector<QueryNode *>>> {
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
  Log(LogLevel::kTrace, "Scheduling preprocessing.");
  // RemoveUnnecessaryTables(graph, tables);

  if (!scheduler_) {
    auto heuristic_choices = GetDefaultHeuristics();
    scheduler_ = std::make_unique<ElasticSchedulingGraphParser>(
        config.pr_hw_library, heuristic_choices.at(config.heuristic_choice),
        first_node_names, drivers, config.use_max_runs_cap,
        config.reduce_single_runs);
  }

  scheduler_->PreprocessNodes(starting_nodes, skipped_nodes, graph, tables);

  auto [min_runs, resulting_plans, scheduling_time, ignored_timeout,
        ignored_stats] = ScheduleAndGetAllPlans(starting_nodes, skipped_nodes,
                                                graph, tables, config);
  Log(LogLevel::kInfo,
      "Main scheduling loop time = " + std::to_string(scheduling_time / 1000) +
          "[milliseconds]");
  std::cout << "PLAN COUNT:" << resulting_plans.size() << std::endl;

  Log(LogLevel::kTrace, "Choosing best plan.");
  // resulting_plans
  auto [best_plan, new_last_config, ignored_data_size, ignored_config_size] =
      plan_evaluator_->GetBestPlan(
          min_runs, current_configuration, config.resource_string,
          config.utilites_scaler, config.config_written_scaler,
          config.utility_per_frame_scaler, resulting_plans,
          config.cost_of_columns, config.streaming_speed,
          config.configuration_speed);
  Log(LogLevel::kTrace, "Creating module queue.");
  // starting_nodes = resulting_plans.at(best_plan).available_nodes;
  skipped_nodes = resulting_plans.at(best_plan).processed_nodes;
  // graph = resulting_plans.at(best_plan).graph;
  // tables = resulting_plans.at(best_plan).tables;

  auto resulting_runs = GetQueueOfResultingRuns(available_nodes, best_plan);
  // available_nodes = FindNewAvailableNodes(starting_nodes, available_nodes);
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "TOTAL SCHEDULING:"
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     start)
                   .count()
            << std::endl;
  Log(LogLevel::kTrace, "Execution plan made!");

  /*for (const auto &run : best_plan) {
    for (const auto &node : run) {
      std::cout << " " << node.node_name << " ";
    }
    std::cout << std::endl;
  }*/

  return resulting_runs;
}

// Method to get node pointers to runs from node names and set composed module
// op params! Only thing missing is the resource elastic information. (Like
// merge sort size)
auto ElasticResourceNodeScheduler::GetQueueOfResultingRuns(
    std::vector<QueryNode *> &available_nodes,
    const std::vector<std::vector<ScheduledModule>> &best_plan)
    -> std::queue<
        std::pair<std::vector<ScheduledModule>, std::vector<QueryNode *>>> {
  // Clear potentially unfinished nodes
  for (const auto &node : available_nodes) {
    node->module_locations.clear();
    if (node->operation_type == QueryOperationType::kMergeSort) {
      node->operation_parameters.operation_parameters.clear();
    }
  }

  // How many nodes in each run?
  std::unordered_map<std::string, int> node_counts;
  const int merge_sort_run_param_count = 2;

  std::queue<std::pair<std::vector<ScheduledModule>, std::vector<QueryNode *>>>
      resulting_runs;
  for (const auto &run : best_plan) {
    std::vector<QueryNode *> chosen_nodes;
    for (int module_index = 0; module_index < run.size(); module_index++) {
      auto chosen_node = GetNodePointerWithName(available_nodes,
                                                run.at(module_index).node_name);
      node_counts[run.at(module_index).node_name] += 1;
      chosen_node->module_locations.push_back(module_index + 1);
      if (std::find(chosen_nodes.begin(), chosen_nodes.end(), chosen_node) ==
          chosen_nodes.end()) {
        chosen_nodes.push_back(chosen_node);
      }
    }
    // Mark end of the run

    for (auto &node : chosen_nodes) {
      if (node->operation_type == QueryOperationType::kMergeSort) {
        auto &merge_op_params = node->operation_parameters.operation_parameters;
        if (!merge_op_params.empty() && merge_op_params.at(0).empty()) {
          merge_op_params.erase(merge_op_params.begin());
        }

        merge_op_params.push_back(
            {node_counts[node->node_name], merge_sort_run_param_count});
        // Find first in run with this node
        auto first_module =
            std::find_if(run.begin(), run.end(), [](auto module) {
              return module.operation_type == QueryOperationType::kMergeSort;
            });
        // TODO: This is wrong!
        auto sorted_status = first_module->processed_table_data;
        for (int i = 0; i < node_counts[node->node_name]; i++) {
          // TODO:Change hardcoded merge sort sizes!
          int capacity = 64;
          std::vector<int> channel_sizes(capacity, 0);
          int offset = i * capacity;

          // If i = 0 then you get the first channel size from the begginning
          // For all others you can just put the 3rd argument
          //

          int starting_point = 0;
          if (i == 0) {
            starting_point++;
            channel_sizes[0] = sorted_status.at(0);
          }
          for (int j = starting_point;
               j < (sorted_status.at(1) + 1) - offset && j < capacity; j++) {
            channel_sizes[j] = sorted_status.at(2);
            if (sorted_status.at(1) == j + offset) {
              int sorted_so_far =
                  sorted_status.at(0) +
                  (sorted_status.at(1) - 1) * sorted_status.at(2);
              channel_sizes[j + 1] =
                  first_module->table_data_size - sorted_so_far;
            }
          }
          merge_op_params.push_back(channel_sizes);
          merge_op_params.push_back({offset});
        }
      }

      node->module_locations.push_back(-1);
      node_counts[node->node_name] = 0;
    }
    resulting_runs.push({run, chosen_nodes});
  }
  return resulting_runs;
}

auto ElasticResourceNodeScheduler::FindNewAvailableNodes(
    std::unordered_set<std::string> &starting_nodes,
    std::vector<QueryNode *> &available_nodes) -> std::vector<QueryNode *> {
  std::vector<QueryNode *> new_available_nodes;
  for (const auto &previous_start_node : available_nodes) {
    previous_start_node->is_finished = true;
  }
  for (const auto &node_name : starting_nodes) {
    auto chosen_node = orkhestrafs::dbmstodspi::ElasticResourceNodeScheduler::
        GetNodePointerWithName(available_nodes, node_name);
    chosen_node->is_finished = false;
    new_available_nodes.push_back(chosen_node);
  }
  return new_available_nodes;
}

auto ElasticResourceNodeScheduler::GetNodePointerWithName(
    std::vector<QueryNode *> &available_nodes, const std::string &node_name)
    -> QueryNode * {
  QueryNode *chosen_node;
  for (const auto &node : available_nodes) {
    chosen_node = FindSharedPointerFromRootNodes(node_name, node);
    if (chosen_node != nullptr) {
      break;
    }
  }
  if (chosen_node == nullptr) {
    throw std::runtime_error("No corresponding node found!");
  }
  return std::move(chosen_node);
}

auto ElasticResourceNodeScheduler::GetDefaultHeuristics()
    -> const std::vector<std::pair<std::vector<std::vector<ModuleSelection>>,
                                   std::vector<std::vector<ModuleSelection>>>> {
  std::vector<std::vector<ModuleSelection>> shortest_first_module_heuristics;
  std::vector<std::vector<ModuleSelection>> longest_first_module_heuristics;
  std::vector<std::vector<ModuleSelection>> shortest_module_heuristics;
  std::vector<std::vector<ModuleSelection>> longest_module_heuristics;
  std::vector<std::vector<ModuleSelection>> all_modules_heuristics;
  shortest_first_module_heuristics.push_back(
      {ModuleSelection(static_cast<std::string>("SHORTEST_AVAILABLE")),
       ModuleSelection(static_cast<std::string>("FIRST_AVAILABLE"))});
  longest_first_module_heuristics.push_back(
      {ModuleSelection(static_cast<std::string>("LONGEST_AVAILABLE")),
       ModuleSelection(static_cast<std::string>("FIRST_AVAILABLE"))});
  shortest_module_heuristics.push_back(
      {ModuleSelection(static_cast<std::string>("SHORTEST_AVAILABLE"))});
  longest_module_heuristics.push_back(
      {ModuleSelection(static_cast<std::string>("LONGEST_AVAILABLE"))});
  all_modules_heuristics.push_back(
      {ModuleSelection(static_cast<std::string>("ALL_AVAILABLE"))});
  return {
      {shortest_first_module_heuristics, longest_first_module_heuristics},
      {shortest_module_heuristics, longest_module_heuristics},
      {shortest_module_heuristics, longest_module_heuristics},
      {all_modules_heuristics, all_modules_heuristics},
      {{}, all_modules_heuristics},
  };
}

auto ElasticResourceNodeScheduler::FindSharedPointerFromRootNodes(
    const std::string &searched_node_name, QueryNode *current_node)
    -> QueryNode * {
  if (current_node->node_name == searched_node_name) {
    return current_node;
  }
  for (const auto &next_node : current_node->next_nodes) {
    if (next_node) {
      auto result =
          FindSharedPointerFromRootNodes(searched_node_name, next_node);
      if (result != nullptr) {
        return result;
      }
    }
  }

  return nullptr;
}
