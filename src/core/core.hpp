﻿/*
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

#include <string>

namespace orkhestrafs::core {
/**
 * @brief Main class to run the whole library
 */
class Core {
 public:
  /**
   * @brief Run the middleware
   * @param input_filename Input definition filename.
   * @param config_filename Config file contianing files to configure the HW.
   */
  static void Run(std::string input_filename, std::string config_filename,
                  bool is_interactive = false);
};
}  // namespace orkhestrafs::core
