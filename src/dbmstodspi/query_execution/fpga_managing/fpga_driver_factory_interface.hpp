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

#include <memory>

#include "accelerator_library_interface.hpp"
#include "fpga_manager_interface.hpp"
#include "memory_manager_interface.hpp"

namespace orkhestrafs::dbmstodspi {

/**
 * @brief Interface class for describing classes which can create driver
 * objects.
 */
class FPGADriverFactoryInterface {
 public:
  virtual ~FPGADriverFactoryInterface() = default;
  virtual auto CreateFPGAManager(AcceleratorLibraryInterface* driver_library)
      -> std::unique_ptr<FPGAManagerInterface> = 0;
  virtual auto CreateAcceleratorLibrary(MemoryManagerInterface* memory_manager)
      -> std::unique_ptr<AcceleratorLibraryInterface> = 0;
};

}  // namespace orkhestrafs::dbmstodspi