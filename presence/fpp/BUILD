# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])

cc_library(
    name = "fpp_manager",
    srcs = [
        "fpp_manager.cc",
    ],
    hdrs = ["fpp_manager.h"],
    visibility = [
        "//presence:__subpackages__",
    ],
    deps = [
        "//internal/platform:logging",
        "//presence:types",
        "//presence/fpp/fpp_c_ffi",
        "//presence/implementation:sensor_fusion",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_test(
    name = "fpp_manager_test",
    size = "small",
    srcs = ["fpp_manager_test.cc"],
    deps = [
        ":fpp_manager",
        "@com_github_protobuf_matchers//protobuf-matchers",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "@platforms//os:windows": [
            "//internal/platform/implementation/windows",
        ],
        "//conditions:default": [
            "//internal/platform/implementation/g3",
        ],
    }),
)
