# veyra-common — shared utilities (config, IPC, logging) used by every target.
# Phase 0: a single trivial translation unit plus the generated version header,
# so downstream targets can already link `veyra::common` and read VEYRA_VERSION.

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/veyra/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/veyra/version.h"
    @ONLY)

add_library(veyra-common STATIC
    src/version.cpp)

add_library(veyra::common ALIAS veyra-common)

target_include_directories(veyra-common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_features(veyra-common PUBLIC cxx_std_20)
