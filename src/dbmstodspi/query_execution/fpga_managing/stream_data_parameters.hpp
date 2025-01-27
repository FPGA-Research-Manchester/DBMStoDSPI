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
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Struct for holding data about how to configure a stream in the DMA
 * engine.
 */
struct StreamDataParameters {
  /// ID
  int stream_id;
  /// How many integers worth of data does a record hold.
  int stream_record_size;
  /// How many records there are in this stream.
  int stream_record_count;
  /// Pointer to the start of the data.
  std::map<volatile uint32_t*, std::vector<int>> physical_addresses_map;
  // volatile uint32_t* physical_address;
  /// How the data should be ordered.
  std::vector<int> stream_specification;
  /// How many chunks are used for each record before the output crossbar.
  int input_chunks_per_record = -1;
  // For multi-channel streams
  /// How many channels can be used for this stream.
  int max_channel_count = -1;
  /// How many records will there be for each channel.
  // std::vector<int> records_per_channel = {-1};
  /// Required for the merge sorting to know how big is the buffer size.
  int smallest_module_size = -1;

  auto operator==(const StreamDataParameters& rhs) const -> bool {
    return stream_id == rhs.stream_id &&
           stream_record_size == rhs.stream_record_size &&
           stream_record_count == rhs.stream_record_count &&
           physical_addresses_map == rhs.physical_addresses_map &&
           stream_specification == rhs.stream_specification &&
           input_chunks_per_record == rhs.input_chunks_per_record &&
           max_channel_count == rhs.max_channel_count;
  }

  auto operator<(const StreamDataParameters& comparable) const -> bool {
    return std::tie(stream_id, stream_record_size, stream_record_count,
                    physical_addresses_map, stream_specification,
                    input_chunks_per_record, max_channel_count) <
           std::tie(comparable.stream_id, comparable.stream_record_size,
                    comparable.stream_record_count,
                    comparable.physical_addresses_map,
                    comparable.stream_specification,
                    comparable.input_chunks_per_record,
                    comparable.max_channel_count);
  }
};

}  // namespace orkhestrafs::dbmstodspi