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

#include "virtual_memory_block.hpp"

using orkhestrafs::dbmstodspi::VirtualMemoryBlock;

VirtualMemoryBlock::~VirtualMemoryBlock() = default;

auto VirtualMemoryBlock::GetVirtualAddress() -> volatile uint32_t* {
  return &memory_area_.at(0);
}

auto VirtualMemoryBlock::GetPhysicalAddress() -> volatile uint32_t* {
  return &memory_area_.at(0);
}

auto VirtualMemoryBlock::GetSize() -> const uint32_t {
  return memory_area_.size() * 4;
}
