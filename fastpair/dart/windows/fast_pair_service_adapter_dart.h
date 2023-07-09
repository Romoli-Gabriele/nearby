// Copyright 2022 Google LLC
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

#ifndef THIRD_PARTY_NEARBY_FASTPAIR_DART_WINDOWS_FAST_PAIR_SERVICE_ADAPTER_DART_H_
#define THIRD_PARTY_NEARBY_FASTPAIR_DART_WINDOWS_FAST_PAIR_SERVICE_ADAPTER_DART_H_

#include <string>

#include "third_party/dart_lang/v2/runtime/include/dart_api_dl.h"
#include "fastpair/dart/windows/fast_pair_service_adapter.h"

namespace nearby {
namespace fastpair {
namespace windows {

// Initiates a default Mediator instance.
DLL_EXPORT void* __stdcall InitMediatorDart();

// Starts scanning service
DLL_EXPORT void __stdcall StartScanDart(void* instance);

// Adds a notification controller observer to the service.
DLL_EXPORT void __stdcall AddNotificationControllerObserverDart(void* instance,
                                                                Dart_Port port);

// Removes a notification controller observer to the service.
DLL_EXPORT void __stdcall RemoveNotificationControllerObserverDart(
    void* instance, Dart_Port port);

// Triggers discovery click action
DLL_EXPORT void __stdcall DiscoveryClickedDart(void* instance, int action);

}  // namespace windows
}  // namespace fastpair
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_FASTPAIR_DART_WINDOWS_FAST_PAIR_SERVICE_ADAPTER_DART_H_
