# ============================================================================
#  Veyra Sounds — top-level build
#
#  Phase 0 goal: every target compiles to a no-op binary so the repository
#  layout and toolchain are proven end-to-end on CI before any real DSP,
#  COM/APO, IPC, or JUCE code lands. Heavy third-party dependencies are
#  declared (see third_party/) but intentionally NOT linked yet; they switch
#  on in later phases via -DVEYRA_FETCH_DEPS=ON.
# ============================================================================
cmake_minimum_required(VERSION 3.25)

# Single source of truth for the product version. Propagated to C++ via
# modules/veyra-common/include/veyra/version.h.in and to installers later.
set(VEYRA_VERSION 0.9.0 CACHE STRING "Veyra Sounds product version")

project(Veyra
    VERSION ${VEYRA_VERSION}
    DESCRIPTION "Veyra Sounds — free, open-source system-wide audio enhancer for Windows"
    HOMEPAGE_URL "https://github.com/NextGenDev-KSK/veyra"
    LANGUAGES C CXX RC)   # C is required by JUCE

# Short git commit (captured at configure time) for the About page / diagnostics.
find_package(Git QUIET)
set(VEYRA_GIT_COMMIT "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _veyra_git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(_veyra_git_commit)
        set(VEYRA_GIT_COMMIT "${_veyra_git_commit}")
    endif()
endif()

# ---- Global language settings ----------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project-wide Win32 hygiene: build against the wide (W) APIs everywhere, and
# stop <windows.h> from defining the min/max macros that collide with the STL.
add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

# Treat sources (and the execution charset) as UTF-8. Our files contain UTF-8
# literals, and fmt/spdlog static-assert that Unicode builds use /utf-8.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Keep all build artifacts in one predictable place so CI and the installer
# scripts can find the four binaries without guessing per-target output dirs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---- Options ---------------------------------------------------------------
# OFF in Phase 0: the no-op stubs must not pull JUCE / RNNoise / etc. so the
# CI build stays fast and green. Each phase that needs a dependency flips this
# on and wires the matching submodule in third_party/CMakeLists.txt.
option(VEYRA_FETCH_DEPS "Configure bundled third-party dependencies" OFF)
option(VEYRA_BUILD_TESTS "Build the test suite" OFF)

if(NOT WIN32)
    message(WARNING "Veyra Sounds targets Windows 10/11 x64. Non-Windows configure is unsupported.")
endif()

# ---- Subprojects -----------------------------------------------------------
add_subdirectory(third_party)
add_subdirectory(modules/veyra-common)
add_subdirectory(modules/veyra-dsp)
add_subdirectory(modules/veyra-rnnoise)

add_subdirectory(apps/veyra-apo)
add_subdirectory(apps/veyra-service)
add_subdirectory(apps/veyra-ui)
add_subdirectory(apps/veyra-overlay)

if(VEYRA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Veyra Sounds ${VEYRA_VERSION} configured (FETCH_DEPS=${VEYRA_FETCH_DEPS}, TESTS=${VEYRA_BUILD_TESTS})")
