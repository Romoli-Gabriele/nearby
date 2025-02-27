// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/platform/pipe.h"

#include "internal/platform/implementation/condition_variable.h"
#include "internal/platform/implementation/mutex.h"
#include "internal/platform/implementation/platform.h"

namespace nearby {

namespace {
using Platform = api::ImplementationPlatform;
}

#pragma push_macro("CreateMutex")
#undef CreateMutex

Pipe::Pipe() {
  auto mutex = Platform::CreateMutex(api::Mutex::Mode::kRegular);
  auto cond = Platform::CreateConditionVariable(mutex.get());
  Setup(std::move(mutex), std::move(cond));
}

#pragma pop_macro("CreateMutex")

}  // namespace nearby
