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

#include "execution_plan_graph_interface.hpp"
#include "gmock/gmock.h"

using orkhestrafs::core_interfaces::ExecutionPlanGraphInterface;

class MockGraph : public ExecutionPlanGraphInterface {
 public:
  MOCK_METHOD(void, DeleteNodes,
              (const std::unordered_set<std::string>& deleted_node_names),
              (override));
  MOCK_METHOD(bool, IsEmpty, (), (override));
  MOCK_METHOD(std::vector<QueryNode*>, GetRootNodesPtrs, (), (override));
  MOCK_METHOD(std::vector<QueryNode*>, GetAllNodesPtrs, (), (override));
  MOCK_METHOD(void, ImportNodes,
              (std::vector<std::unique_ptr<QueryNode>> new_nodes), (override));
};