#pragma once
#include <cstdint>
#include <vector>
struct StreamDataParameters {
  const int stream_id;
  const int stream_record_size;
  const int stream_record_count;
  const volatile uint32_t* physical_address;
  const std::vector<int> stream_specification;

  auto operator<(const StreamDataParameters& comparable) const -> bool {
    return stream_id < comparable.stream_id;
  }
};