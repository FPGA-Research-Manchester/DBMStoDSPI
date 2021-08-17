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
#include <array>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "data_manager.hpp"
#include "memory_block_interface.hpp"
#include "memory_manager.hpp"
#include "query_acceleration_constants.hpp"
#include "stream_data_parameters.hpp"
#include "table_data.hpp"

using dbmstodspi::data_managing::table_data::TableData;

namespace dbmstodspi::query_managing {

/**
 * @brief Class to handle the input and output data, its parameters and tables.
 */
class TableManager {
 public:
  /**
   * @brief Get the length of a record from the given table.
   * @param input_table Information about a data table.
   * @return Size of a record in the given table in 32 bit integers.
   */
  static auto GetRecordSizeFromTable(const TableData& input_table) -> int;
  /**
   * @brief Read data from the memory mapped DDR after acceleration.
   * @param output_stream_parameters Query operation parameters for the output
   * data.
   * @param output_tables Results of the query nodes' execution phase
   * @param result_record_counts How many records should be read from the DDR.
   * @param allocated_memory_blocks Vector of allocated memory blocks.
   */
  static void ReadResultTables(
      const std::vector<fpga_managing::StreamDataParameters>&
          output_stream_parameters,
      std::vector<TableData>& output_tables,
      const std::array<
          int, fpga_managing::query_acceleration_constants::kMaxIOStreamCount>&
          result_record_counts,
      std::vector<std::unique_ptr<fpga_managing::MemoryBlockInterface>>&
          allocated_memory_blocks);

  /**
   * @brief Write the table to a CSV file with a timestamp
   * @param data_table Resulting data to be written to the file.
   * @param filename String of the name of the file to be written to.
   */
  static void WriteResultTableFile(const TableData& data_table,
                                   std::string filename);


  static auto WriteDataToMemory(
      const data_managing::DataManager& data_manager,
      const std::vector<std::vector<int>>& stream_specification,
      int stream_index,
      const std::unique_ptr<fpga_managing::MemoryBlockInterface>& memory_device,
      std::string filename) -> std::pair<int, int>;



  static auto ReadTableFromMemory(
      const data_managing::DataManager& data_manager,
      const std::vector<std::vector<int>>& stream_specification,
      int stream_index,
      const std::unique_ptr<fpga_managing::MemoryBlockInterface>& memory_device,
      int row_count) -> TableData;
  static auto ReadTableFromFile(
      const data_managing::DataManager& data_manager,
      const std::vector<std::vector<int>>& stream_specification,
      int stream_index, std::string filename) -> TableData;
  static auto GetColumnDefsVector(
      const data_managing::DataManager& data_manager,
      std::vector<std::vector<int>> node_parameters, int stream_index)
      -> std::vector<std::pair<ColumnDataType, int>>;

 private:
  static void ReadOutputDataFromMemoryBlock(
      const std::unique_ptr<fpga_managing::MemoryBlockInterface>& output_device,
      TableData& resulting_table, const int& result_size);
  static void WriteInputDataToMemoryBlock(
      const std::unique_ptr<fpga_managing::MemoryBlockInterface>& input_device,
      const TableData& input_table, int previous);
  static void PrintWrittenData(
      const std::string& table_name,
      const std::unique_ptr<fpga_managing::MemoryBlockInterface>& input_device,
      const TableData& input_table);
  static void PrintDataSize(const TableData& data_table);
  static auto GetColumnDataTypesFromSpecification(
      const std::vector<std::vector<int>>& stream_specification,
      int stream_index) -> std::vector<ColumnDataType>;
};

}  // namespace dbmstodspi::query_managing