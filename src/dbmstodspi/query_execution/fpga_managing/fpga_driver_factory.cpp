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

#include "fpga_driver_factory.hpp"

#include <map>
#include <memory>

#include "accelerator_library.hpp"
#include "addition_setup.hpp"
#include "aggregation_sum_setup.hpp"
#include "dma_setup.hpp"
#include "filter_setup.hpp"
#include "fpga_driver_factory.hpp"
#include "fpga_manager.hpp"
#include "join_setup.hpp"
#include "linear_sort_setup.hpp"
#include "merge_sort_setup.hpp"
#include "multiplication_setup.hpp"
#include "sobel_setup.hpp"
#include "black_white_setup.hpp"
#include "operation_types.hpp"

using orkhestrafs::core_interfaces::operation_types::QueryOperationType;
using orkhestrafs::dbmstodspi::FPGADriverFactory;

auto FPGADriverFactory::CreateFPGAManager(
    AcceleratorLibraryInterface* driver_library)
    -> std::unique_ptr<FPGAManagerInterface> {
  return std::make_unique<FPGAManager>(driver_library);
}
auto FPGADriverFactory::CreateAcceleratorLibrary(
    MemoryManagerInterface* memory_manager)
    -> std::unique_ptr<AcceleratorLibraryInterface> {
  std::map<QueryOperationType,
           std::unique_ptr<AccelerationModuleSetupInterface>>
      module_driver_library;
  module_driver_library.insert(
      {QueryOperationType::kFilter, std::make_unique<FilterSetup>()});
  module_driver_library.insert(
      {QueryOperationType::kJoin, std::make_unique<JoinSetup>()});
  module_driver_library.insert(
      {QueryOperationType::kMergeSort, std::make_unique<MergeSortSetup>()});
  module_driver_library.insert(
      {QueryOperationType::kLinearSort, std::make_unique<LinearSortSetup>()});
  module_driver_library.insert(
      {QueryOperationType::kAddition, std::make_unique<AdditionSetup>()});
  module_driver_library.insert({QueryOperationType::kMultiplication,
                                std::make_unique<MultiplicationSetup>()});
  module_driver_library.insert({QueryOperationType::kAggregationSum,
                                std::make_unique<AggregationSumSetup>()});
  module_driver_library.insert({QueryOperationType::kSobel,
                                std::make_unique<SobelSetup>()});
  module_driver_library.insert({QueryOperationType::kBlackWhite,
                                std::make_unique<BlackWhiteSetup>()});
  return std::make_unique<AcceleratorLibrary>(memory_manager,
                                              std::make_unique<DMASetup>(),
                                              std::move(module_driver_library));
}
