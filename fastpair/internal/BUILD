licenses(["notice"])

cc_library(
    name = "internal",
    srcs = ["fast_pair_seeker_impl.cc"],
    hdrs = [
        "fast_pair_seeker_impl.h",
    ],
    compatible_with = ["//buildenv/target:non_prod"],
    visibility = [
        "//fastpair:__subpackages__",
    ],
    deps = [
        "//fastpair:fast_pair_controller",
        "//fastpair:fast_pair_events",
        "//fastpair:fast_pair_seeker",
        "//fastpair/internal/mediums",
        "//fastpair/pairing",
        "//fastpair/repository:device_repository",
        "//fastpair/retroactive",
        "//fastpair/scanning:scanner",
        "//internal/platform:types",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "fast_pair_seeker_impl_test",
    size = "small",
    srcs = [
        "fast_pair_seeker_impl_test.cc",
    ],
    deps = [
        ":internal",
        "//fastpair:fast_pair_events",
        "//fastpair/message_stream:fake_gatt_callbacks",
        "//fastpair/message_stream:fake_provider",
        "//fastpair/repository:test_support",
        "//internal/platform:test_util",
        "//internal/platform:types",
        "//internal/platform/implementation/g3",  # build_cleaner: keep
        "@com_github_protobuf_matchers//protobuf-matchers",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
