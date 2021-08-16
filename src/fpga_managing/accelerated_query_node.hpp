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

#include "accelerated_query_node.hpp"
#include "operation_types.hpp"
#include "stream_data_parameters.hpp"

namespace dbmstodspi::fpga_managing {

/**
 * @brief Struct to hold all of the important information about a query node to
 * accelerate it with an FPGA.
 */
struct AcceleratedQueryNode {
  const std::vector<StreamDataParameters> input_streams;
  const std::vector<StreamDataParameters> output_streams;
  const operation_types::QueryOperationType operation_type;
  const int operation_module_location;
  const std::vector<std::vector<int>> operation_parameters;

  // auto operator==(const AcceleratedQueryNode& rhs) const -> bool {
  //  return input_streams == rhs.input_streams &&
  //         output_streams == rhs.output_streams &&
  //         operation_type == rhs.operation_type &&
  //         operation_parameters == rhs.operation_parameters;
  //}
};

}  // namespace dbmstodspi::fpga_managing