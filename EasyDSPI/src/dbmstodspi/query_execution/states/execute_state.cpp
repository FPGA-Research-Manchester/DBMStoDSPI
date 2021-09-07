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

#include "execute_state.hpp"
#include "setup_nodes_state.hpp"

#include <iostream>
#include <stdexcept>

using easydspi::dbmstodspi::ExecuteState;
using easydspi::dbmstodspi::GraphProcessingFSMInterface;
using easydspi::dbmstodspi::StateInterface;
using easydspi::dbmstodspi::SetupNodesState;

std::unique_ptr<StateInterface> ExecuteState::execute(
    GraphProcessingFSMInterface* fsm) {
  if (fsm->IsRunReadyForExecution()) {
    if (!fsm->IsRunValid()) {
        // Fix stuff
      throw std::runtime_error("Not implemented");
    }
  } else {
    throw std::runtime_error("No nodes ready to execute!");
  }

  fsm->ExecuteAndProcessResults();

  std::cout << "Execute" << std::endl;

  return std::make_unique<SetupNodesState>();
}