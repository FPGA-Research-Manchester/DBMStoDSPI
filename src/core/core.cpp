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

#include "core.hpp"

#include "core_execution/execution_manager_factory.hpp"
#include "core_input/input_manager_factory.hpp"
#include "input_manager_interface.hpp"

using orkhestrafs::core::Core;
using orkhestrafs::core::core_execution::ExecutionManagerFactory;
using orkhestrafs::core::core_input::InputManagerFactory;

void Core::Run(std::string input_filename, std::string config_filename, bool is_interactive) {
  // Could be done as a one liner but additional future logic is expected here
  // to be passed to the factories.
  auto input = InputManagerFactory::GetManager()->Parse(
      std::move(input_filename), std::move(config_filename));
  ExecutionManagerFactory::GetManager(input.second, is_interactive)
      ->Execute(std::move(input.first));
}