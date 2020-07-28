#pragma once
#include <cstdint>

#include "AccelerationModule.hpp"

class MockAccelerationModule : public AccelerationModule {
 public:
  MockAccelerationModule(int* volatile ctrl_ax_ibase_address,
                         uint32_t module_position);
  ~MockAccelerationModule() override;
  void writeToModule(uint32_t module_internal_address, uint32_t write_data);
  auto readFromModule(uint32_t module_internal_address) -> uint32_t;
};