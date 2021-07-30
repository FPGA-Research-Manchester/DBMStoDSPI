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

#include <memory>

#include "config_value_checker_interface.hpp"
#include "graph_creator.hpp"
#include "graph_creator_interface.hpp"
#include "input_config_reader_interface.hpp"
#include "input_manager_interface.hpp"
#include "json_reader_interface.hpp"
#include "json_validator_interface.hpp"

using easydspi::core_interfaces::Config;
using easydspi::core_interfaces::ExecutionPlanGraphInterface;
using easydspi::core_interfaces::InputManagerInterface;
using easydspi::dbmstodspi::ConfigValueCheckerInterface;
using easydspi::dbmstodspi::GraphCreatorInterface;
using easydspi::dbmstodspi::InputConfigReaderInterface;
using easydspi::dbmstodspi::JSONReaderInterface;
using easydspi::dbmstodspi::JSONValidatorInterface;

namespace easydspi::core::core_input {

class InputManager : public InputManagerInterface {
 private:
  std::unique_ptr<GraphCreatorInterface> graph_creator_;
  std::unique_ptr<JSONValidatorInterface> json_validator_;
  std::unique_ptr<JSONReaderInterface> json_reader_;
  std::unique_ptr<ConfigValueCheckerInterface> config_validator_;
  std::unique_ptr<InputConfigReaderInterface> input_config_reader_;

 public:
  ~InputManager() override = default;
  InputManager(std::unique_ptr<GraphCreatorInterface> graph_creator,
               std::unique_ptr<JSONReaderInterface> json_reader,
               std::unique_ptr<InputConfigReaderInterface> input_config_reader,
               std::unique_ptr<JSONValidatorInterface> json_validator,
               std::unique_ptr<ConfigValueCheckerInterface> convig_validator)
      : graph_creator_{std::move(graph_creator)},
        json_reader_{std::move(json_reader)},
        input_config_reader_{std::move(input_config_reader)},
        json_validator_{std::move(json_validator)},
        config_validator_{std::move(convig_validator)} {};

  std::pair<std::unique_ptr<ExecutionPlanGraphInterface>, Config> parse(
      std::string input_filename, std::string config_filename) override;
};
}  // namespace easydspi::core::core_input