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

#include "acceleration_module_setup_interface.hpp"
#include "aggregation_sum_interface.hpp"

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Class to calculate the aggregating global sum acceleration module
 * setup.
 */
class AggregationSumSetup : public AccelerationModuleSetupInterface {
 private:
  static void SetupPassthroughSum(AggregationSumInterface& aggregation_module);

 public:
  void SetupModule(AccelerationModule& acceleration_module,
                   const AcceleratedQueryNode& module_parameters) override;
  auto CreateModule(MemoryManagerInterface* memory_manager, int module_postion)
      -> std::unique_ptr<AccelerationModule> override;
  /**
   * @brief Setup the configuration for the global sum module.
   *
   * The operation parameters just consist of the chunk ID where the aggregation
   * should take place. The second vector is for read_back specification to know
   * which positions should get read.
   *
   * @param aggregation_module Module instance to have access to the memory
   * mapped registers.
   * @param stream_id Which stream the aggregation operates on.
   * @param operation_parameters Data to specify how the operation should be
   * configured
   */
  static void SetupAggregationSum(
      AggregationSumInterface& aggregation_module, int stream_id,
      const std::vector<std::vector<int>>& operation_parameters);
};

}  // namespace orkhestrafs::dbmstodspi