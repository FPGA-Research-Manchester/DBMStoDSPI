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
#include "acceleration_module.hpp"
#include "aggregation_sum_interface.hpp"
#include "memory_manager_interface.hpp"
#include "read_back_module.hpp"

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Class which implements low level memory writes to the aggregating
 * global sum operation accelerator.
 */
class AggregationSum : public AggregationSumInterface, public ReadBackModule {
 private:
 public:
  ~AggregationSum() override = default;
  /**
   * @brief Constructor to set the memory manager instance to access memory
   * mapped registers.
   * @param memory_manager Memory manager instance to access memory mapping.
   * @param module_position Position of the module in the bitstream.
   */
  explicit AggregationSum(MemoryManagerInterface* memory_manager,
                          int module_position)
      : ReadBackModule(memory_manager, module_position){};

  /**
   * @brief Set the module to start prefetching data.
   *
   * Two ways to use this:
   * 1) Set request data to 0 and let the DMA output controller drive this
   * module. Then destroy data has to be 0 as well. 2) Set request data to 1 and
   * destroy the data. Then the count data value can be read to see if the
   * module is active or not and the output controller shouldn't be started.
   *
   * @param request_data Set the module request data itself.
   * @param destroy_data Should the input stream be destroyed or passed through.
   * @param count_data Set the module to count data.
   */
  void StartPrefetching(bool request_data, bool destroy_data,
                        bool count_data) override;
  /**
   * @brief Configure the module to aggregate a specific stream and chunk.
   * @param stream_id Input stream ID.
   * @param chunk_id ID of the chunk to aggregate.
   */
  void DefineInput(int stream_id, int chunk_id) override;
  /**
   * @brief Read the resulting sum from the required location.
   * @param data_position Position of the data element.
   * @param is_low Should the low bits be read instead of the high bits?
   * @return The sum value from the given location.
   */
  auto ReadSum(int data_position, bool is_low) -> uint32_t override;
  /**
   * @brief Read the result of the read back module using the #ReadSum method.
   * @param data_position Position of the data to be read.
   * @return 32bit integer value read from the result register.
   */
  auto ReadResult(int data_position) -> uint32_t override;
  /**
   * @brief Read the command register to see if it is still active
   * @return Boolean noting if the module is still active when the output
   * controller isn't driving the data.
   */
  auto IsModuleActive() -> bool override;
  /**
   * @brief Reset all registers to 0.
   */
  void ResetSumRegisters() override;
};

}  // namespace orkhestrafs::dbmstodspi