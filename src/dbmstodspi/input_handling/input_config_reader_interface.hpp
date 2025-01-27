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
#include <map>
#include <string>

namespace orkhestrafs::dbmstodspi {
/**
 * @brief Interface class describing a class reading a config file.
 */
class InputConfigReaderInterface {
 public:
  virtual ~InputConfigReaderInterface() = default;
  /**
   * @brief Parse the given config file.
   * @param filename Config filename.
   * @return Return the name of the field and the given value map.
   */
  virtual auto ParseInputConfig(const std::string& filename)
      -> std::map<std::string, std::string> = 0;
};

}  // namespace orkhestrafs::dbmstodspi