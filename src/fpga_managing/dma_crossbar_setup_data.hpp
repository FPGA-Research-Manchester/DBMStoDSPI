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
#include <vector>

#include "query_acceleration_constants.hpp"

namespace dbmstodspi::fpga_managing {

/**
 * @brief Struct to hold crossbar configuration data vectors.
 */
struct DMACrossbarSetupData {
  /**
   * @brief Constructor to initialise default configuration data.
   */
  DMACrossbarSetupData()
      : chunk_selection(query_acceleration_constants::kDatapathWidth,
                        (query_acceleration_constants::kDatapathLength - 1)),
        position_selection(query_acceleration_constants::kDatapathWidth, 0) {}
  /// Data which helps choose the chunk position.
  std::vector<int> chunk_selection;
  /// Data which helps choose the position within a chunk.
  std::vector<int> position_selection;
};

}  // namespace dbmstodspi::fpga_managing