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

#include "csv_reader_interface.hpp"
#include "gmock/gmock.h"
#include "memory_block_interface.hpp"

using orkhestrafs::core_interfaces::MemoryBlockInterface;
using orkhestrafs::core_interfaces::table_data::ColumnDataType;
using orkhestrafs::dbmstodspi::CSVReaderInterface;

class MockCSVReader : public CSVReaderInterface {
 private:
  using ColumDefsVector = std::vector<std::pair<ColumnDataType, int>>;

 public:
  MOCK_METHOD(std::vector<std::vector<std::string>>, ReadTableData,
              (const std::string& filename, char separator,
               int& rows_already_read),
              (override));
  MOCK_METHOD(int, WriteTableFromFileToMemory,
              (const std::string& filename, char separator,
               const ColumDefsVector& column_defs_vector,
               MemoryBlockInterface* memory_device),
              ());
};