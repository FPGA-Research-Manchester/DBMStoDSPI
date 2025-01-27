﻿/*
Copyright 2022 University of Manchester

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

#include "benchmark_schedule_state.hpp"

#include "logger.hpp"
#include "print_benchmark_state.hpp"

using orkhestrafs::dbmstodspi::BenchmarkScheduleState;
using orkhestrafs::dbmstodspi::PrintBenchmarkState;
using orkhestrafs::dbmstodspi::logging::Log;
using orkhestrafs::dbmstodspi::logging::LogLevel;

// TODO(Kaspar): Duplicates normal schedule state! Need to have a constructor to
// tell what is the next state
auto BenchmarkScheduleState::Execute(GraphProcessingFSMInterface* fsm)
    -> std::unique_ptr<StateInterface> {
  Log(LogLevel::kTrace, "Benchmark schedule state");
  fsm->UpdateAvailableNodesGraph();
  if (fsm->IsBenchmarkDone()) {
    return std::make_unique<PrintBenchmarkState>();
  }
  fsm->BenchmarkScheduleUnscheduledNodes();

  return std::make_unique<BenchmarkScheduleState>();
}