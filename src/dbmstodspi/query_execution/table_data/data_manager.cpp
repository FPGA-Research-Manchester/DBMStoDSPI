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

#include "data_manager.hpp"

#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

#include "csv_reader.hpp"
#include "logger.hpp"
#include "types_converter.hpp"
#include "util.hpp"

using orkhestrafs::core_interfaces::table_data::ColumnDataType;
using orkhestrafs::core_interfaces::table_data::TableData;
using orkhestrafs::dbmstodspi::DataManager;
using orkhestrafs::dbmstodspi::logging::Log;
using orkhestrafs::dbmstodspi::logging::LogLevel;
using orkhestrafs::dbmstodspi::logging::ShouldLog;
using orkhestrafs::dbmstodspi::util::IsValidFile;

auto DataManager::GetHeaderColumnVector(
    const std::vector<ColumnDataType>& column_data_types,
    const std::vector<int>& column_sizes) const
    -> std::vector<std::pair<ColumnDataType, int>> {
  std::vector<std::pair<ColumnDataType, int>> table_column_label_vector;
  // Assuming data type vector and column size vectors are the same length
  // (Change them into a single vector of pairs or sth like that.)
  for (int i = 0; i < column_data_types.size(); i++) {
    double data_type_size =
        data_type_sizes_.at(column_data_types.at(i)) * column_sizes.at(i);
    if (data_type_size != static_cast<int>(data_type_size)) {
      throw std::runtime_error(column_sizes.at(i) + "size is not supported!");
    }

    table_column_label_vector.emplace_back(column_data_types.at(i),
                                           static_cast<int>(data_type_size));
  }

  return table_column_label_vector;
}

auto DataManager::WriteDataFromCSVToMemory(
    const std::string& filename,
    const std::vector<std::pair<ColumnDataType, int>>& column_defs_vector,
    MemoryBlockInterface* memory_device) const -> int {
  return csv_reader_->WriteTableFromFileToMemory(
      filename, separator_, column_defs_vector,
      memory_device);  // Make the CSVReader and this function return the record
                       // size as well as it is already calculated in the
                       // function.
}

// Rest of the methods should get eventually removed!

auto DataManager::ParseDataFromCSV(
    const std::string& filename,
    const std::vector<ColumnDataType>& column_data_types,
    const std::vector<int>& column_sizes, int& rows_already_read) const
    -> TableData {
  int record_size = 0;
  std::vector<std::pair<ColumnDataType, int>> table_column_label_vector;
  // Assuming data type vector and column size vectors are the same length
  for (int i = 0; i < column_data_types.size(); i++) {
    double data_type_size =
        data_type_sizes_.at(column_data_types.at(i)) * column_sizes.at(i);
    if (data_type_size != static_cast<int>(data_type_size)) {
      throw std::runtime_error(column_sizes.at(i) + "size is not supported!");
    }

    table_column_label_vector.emplace_back(column_data_types.at(i),
                                           static_cast<int>(data_type_size));
    record_size += static_cast<int>(data_type_size);
  }

  TableData table_data;
  table_data.table_column_label_vector = table_column_label_vector;

  if (IsValidFile(filename)) {
    auto data_rows =
        csv_reader_->ReadTableData(filename, separator_, rows_already_read);
    table_data.table_data_vector.reserve(record_size * data_rows.size());
    TypesConverter::AddIntegerDataFromStringData(
        data_rows, table_data.table_data_vector, table_column_label_vector);
  }  // There should be an else here?

  return table_data;
}

void DataManager::AddStringDataFromIntegerData(
    const std::vector<uint32_t>& integer_data,
    std::vector<std::vector<std::string>>& string_data,
    const std::vector<std::pair<ColumnDataType, int>>& data_types_vector) {
  TypesConverter::AddStringDataFromIntegerData(integer_data, string_data,
                                               data_types_vector);
}

void DataManager::PrintStringData(
    const std::vector<std::vector<std::string>>& string_data) const {
  auto log_level = LogLevel::kDebug;
  if (ShouldLog(log_level)) {
    std::stringstream ss;
    ss << "data: " << std::endl;
    for (const auto& row : string_data) {
      for (const auto& element : row) {
        if (&element != &row.back()) {
          ss << element << separator_;
        } else {
          ss << element << std::endl;
        }
      }
    }
    Log(log_level, ss.str());
  }
}

void DataManager::PrintTableData(const TableData& table_data) const {
  std::vector<std::vector<std::string>> string_data_vector;
  DataManager::AddStringDataFromIntegerData(
      table_data.table_data_vector, string_data_vector,
      table_data.table_column_label_vector);
  DataManager::PrintStringData(string_data_vector);
}

void DataManager::WriteBinaryFile(const std::vector<uint32_t>& binary_data,
    const std::string& filename) {
  if (!binary_data.empty()) {
    std::ofstream output_file(filename, std::ios::out | std::ios::binary);
    if (!output_file) {
      throw std::runtime_error("Can't write " + filename);
    }
    /*for (int i = 0; i < binary_data.size(); i += 16) {
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+12]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+13]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+14]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+15]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+8]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+9]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+10]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+11]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+4]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+5]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+6]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+7]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+1]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+2]), 4);
      output_file.write(reinterpret_cast<const char*>(&binary_data[i+3]), 4);
    }*/
    std::array<char, 4> new_word;
    for (const auto& word : binary_data) {
      auto word_ptr = reinterpret_cast<const char*>(&word);
      new_word = {word_ptr[3], word_ptr[2], word_ptr[1], word_ptr[0]};
      output_file.write(reinterpret_cast<char*>(&new_word), 4);
    }
    /*output_file.write(reinterpret_cast<const char*>(&binary_data[0]),
                      binary_data.size() * sizeof(uint32_t));*/
    output_file.close();
  }
  // Not bothering with empty binary files
}

void DataManager::WriteTableData(const TableData& table_data,
                                 const std::string& filename) const {
  if (table_data.table_column_label_vector.size() == 1 &&
      table_data.table_column_label_vector.front().first ==
          ColumnDataType::kPixel) {
    WriteBinaryFile(table_data.table_data_vector, filename);
  } else {
    std::vector<std::vector<std::string>> string_data_vector;
    DataManager::AddStringDataFromIntegerData(
        table_data.table_data_vector, string_data_vector,
        table_data.table_column_label_vector);
    std::ofstream output_file(filename, std::ios::trunc);
    if (output_file.is_open()) {
      for (const auto& line : string_data_vector) {
        for (auto iter = line.begin(); iter != line.end(); iter++) {
          if (iter != line.begin()) {
            output_file << separator_;
          }
          output_file << *iter;
        }
        output_file << std::endl;
      }
      output_file.close();
    } else {
      throw std::runtime_error("Can't open " + filename);
    }
  }
}

void DataManager::WriteRawTableData(const TableData& table_data,
                                    const std::string& filename) const {
  auto row_size =
      std::accumulate(table_data.table_column_label_vector.begin(),
                      table_data.table_column_label_vector.end(), 0,
                      [](int sum, auto column) { return column.second + sum; });

  std::ofstream output_file(filename, std::ios::trunc);
  if (output_file.is_open()) {
    for (int i = 0; i < table_data.table_data_vector.size(); i++) {
      if (i % row_size == 0) {
        output_file << std::endl;
      }
      output_file << table_data.table_data_vector.at(i) << " ";
    }
    output_file << std::endl;
    output_file.close();
  } else {
    throw std::runtime_error("Can't open " + filename);
  }
}