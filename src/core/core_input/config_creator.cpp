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

#include "config_creator.hpp"

#include <algorithm>
#include <vector>

#include <sstream>
#include <iostream>

#include "pr_module_data.hpp"
#include "query_scheduling_data.hpp"
#include "table_data.hpp"

using orkhestrafs::core::core_input::ConfigCreator;

using orkhestrafs::core_interfaces::hw_library::PRModuleData;
using orkhestrafs::core_interfaces::operation_types::QueryOperation;
using orkhestrafs::core_interfaces::query_scheduling_data::kSupportedFunctions;
using orkhestrafs::core_interfaces::table_data::kDataTypeNames;
using orkhestrafs::core_interfaces::table_data::SortedSequence;

auto ConfigCreator::GetConfig(const std::string& config_filename) -> Config {
  std::string configurations_library = "CONFIGURATIONS_LIBRARY";
  std::string memory_requirements = "BITSTREAMS_MEM_REQ";
  std::string data_type_sizes = "DATA_SIZE_CONFIG";
  std::string data_separator = "DATA_SEPARATOR";
  std::string table_metadata = "TABLE_METADATA";
  std::string hw_library = "HW_LIBRARY";

  std::string temp = "TEMP";

  // repo.json is hardcoded for now.

  auto config_values = config_reader_->ParseInputConfig(config_filename);

  auto accelerator_library_data = json_reader_->ReadAcceleratorLibrary(
      config_values[configurations_library]);

  Config config;

  std::stringstream ss(config_values[temp]);

  std::string s;
  while (std::getline(ss, s, ',')) {
    config.temp.push_back(s);
  }

  config.accelerator_library =
      ConvertStringMapToQueryOperations(accelerator_library_data);
  config.module_library =
      ConvertAcceleratorLibraryToModuleLibrary(accelerator_library_data);

  auto all_tables_json_data =
      json_reader_->ReadAllTablesData(config_values[table_metadata]);

  config.initial_all_tables_metadata = CreateTablesData(all_tables_json_data);

  auto hw_library_json_data =
      json_reader_->ReadHWLibraryData(config_values[hw_library]);

  config.pr_hw_library = CreateHWLibrary(hw_library_json_data);

  auto string_key_data_sizes =
      json_reader_->ReadDataSizes(config_values[data_type_sizes]);
  for (const auto& [string_key, size_scale] : string_key_data_sizes) {
    config.data_sizes.insert({kDataTypeNames.at(string_key), size_scale});
  }

  config.required_memory_space =
      json_reader_->ReadReqMemorySpace(config_values[memory_requirements]);
  config.csv_separator = config_values[data_separator].c_str()[0];
  return config;
}

auto ConfigCreator::ConvertStringMapToQueryOperations(
    const std::map<std::vector<std::pair<std::string, std::vector<int>>>,
                   std::string>& string_map)
    -> std::map<std::vector<QueryOperation>, std::string> {
  std::map<std::vector<QueryOperation>, std::string> resulting_map;
  for (const auto& [vector_key, string_value] : string_map) {
    std::vector<QueryOperation> query_vector;
    for (const auto& operation : vector_key) {
      query_vector.emplace_back(kSupportedFunctions.at(operation.first),
                                operation.second);
    }
    resulting_map.insert({query_vector, string_value});
  }
  return resulting_map;
}

auto ConfigCreator::CreateTablesData(
    const std::vector<
        std::map<std::string, std::variant<std::string, int,
                                           std::vector<std::vector<int>>>>>&
        tables_data_in_string_form) -> std::map<std::string, TableMetadata> {
  std::map<std::string, TableMetadata> resulting_table_meta_data;

  std::string filename_field = "filename";
  std::string record_size_field = "record_size";
  std::string record_count_field = "record_count";
  std::string sorted_status_field = "sorted_status";

  for (const auto& table_meta_data_map : tables_data_in_string_form) {
    TableMetadata current_table;
    std::string filename =
        std::get<std::string>(table_meta_data_map.at(filename_field));
    current_table.record_count =
        std::get<int>(table_meta_data_map.at(record_count_field));
    current_table.record_size =
        std::get<int>(table_meta_data_map.at(record_size_field));
    for (const auto& sorted_sequence : std::get<std::vector<std::vector<int>>>(
             table_meta_data_map.at(sorted_status_field))) {
      SortedSequence current_sequence = {sorted_sequence.at(0),
                                         sorted_sequence.at(1)};
      current_table.sorted_status.push_back(current_sequence);
    }
    resulting_table_meta_data.insert({filename, std::move(current_table)});
  }
  return resulting_table_meta_data;
}

auto ConfigCreator::CreateHWLibrary(
    const std::map<
        std::string,
        std::pair<
            std::map<std::string,
                     std::map<std::string, std::variant<std::vector<int>, int,
                                                        std::string>>>,
            std::vector<std::vector<std::string>>>>& hw_data_in_string_form)
    -> std::map<QueryOperationType, OperationPRModules> {
  std::string locations_field = "locations";
  std::string length_field = "length";
  std::string capacity_field = "capacity";
  std::string resource_string_field = "string";
  std::string is_backwards_field = "is_backwards";

  std::map<QueryOperationType, OperationPRModules> resulting_library;

  for (const auto& [operation_string, pr_module_data] :
       hw_data_in_string_form) {
    std::map<std::string, PRModuleData> bitstream_map;
    for (const auto& [bitstream_name, bitstream_parameters] :
         pr_module_data.first) {
      PRModuleData current_module;
      current_module.capacity =
          std::get<std::vector<int>>(bitstream_parameters.at(capacity_field));
      current_module.fitting_locations =
          std::get<std::vector<int>>(bitstream_parameters.at(locations_field));
      current_module.is_backwards =
          std::get<int>(bitstream_parameters.at(is_backwards_field));
      current_module.length =
          std::get<int>(bitstream_parameters.at(length_field));
      current_module.resource_string =
          std::get<std::string>(bitstream_parameters.at(resource_string_field));

      bitstream_map.insert({bitstream_name, current_module});
    }
    OperationPRModules current_module_set;
    current_module_set.bitstream_map = bitstream_map;
    current_module_set.starting_locations = pr_module_data.second;
    resulting_library.insert(
        {kSupportedFunctions.at(operation_string), current_module_set});
  }
  return resulting_library;
}

auto ConfigCreator::ConvertAcceleratorLibraryToModuleLibrary(
    const std::map<std::vector<std::pair<std::string, std::vector<int>>>,
                   std::string>& accelerator_library_data)
    -> std::map<QueryOperationType, std::vector<std::vector<int>>> {
  std::map<QueryOperationType, std::vector<std::vector<int>>> resulting_map;
  for (const auto& [vector_key, string_value] : accelerator_library_data) {
    for (const auto& operation : vector_key) {
      if (auto search =
              resulting_map.find(kSupportedFunctions.at(operation.first));
          search != resulting_map.end()) {
        for (int param_index = 0; param_index < operation.second.size();
             param_index++) {
          search->second[param_index].push_back(operation.second[param_index]);
        }
      } else {
        std::vector<std::vector<int>> operation_params;
        for (int param_index : operation.second) {
          operation_params.push_back({param_index});
        }
        resulting_map.insert(
            {kSupportedFunctions.at(operation.first), operation_params});
      }
    }
  }

  for (auto& [operation, params] : resulting_map) {
    for (auto& param_vector : params) {
      std::sort(param_vector.begin(), param_vector.end());
      param_vector.erase(std::unique(param_vector.begin(), param_vector.end()),
                         param_vector.end());
    }
  }

  return resulting_map;
}