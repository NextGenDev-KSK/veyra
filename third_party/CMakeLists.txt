# ============================================================================
#  third_party — bundled dependency wiring
#
#  Phase 0: this is a no-op. The stub binaries depend on nothing here, so we
#  do not require any submodule to be checked out to configure or build.
#
#  When VEYRA_FETCH_DEPS=ON (later phases), each dependency is brought in via
#  add_subdirectory on its submodule path. The guards below let a phase enable
#  exactly the deps it needs and fail with a clear message if the submodule
#  has not been initialised.
# ============================================================================

# --- nlohmann/json (vendored single header, MIT, always available) ---------
# Used by veyra-common for config (de)serialisation. Header-only, so it needs
# no submodule and is wired unconditionally. Consumers #include <nlohmann/json.hpp>.
add_library(veyra_nlohmann_json INTERFACE)
add_library(veyra::json ALIAS veyra_nlohmann_json)
target_include_directories(veyra_nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# --- Catch2 v2 (vendored single header, BSL-1.0, test-only) ----------------
# Consumers #include <catch2/catch.hpp>. Header-only; needed only when
# VEYRA_BUILD_TESTS is on, but wiring the target unconditionally is harmless.
add_library(veyra_catch2 INTERFACE)
add_library(veyra::catch2 ALIAS veyra_catch2)
target_include_directories(veyra_catch2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# --- spdlog (submodule, MIT, header-only mode) -----------------------------
# Used by veyra-common's logger. Header-only (no SPDLOG_COMPILED_LIB), so this
# is just an include path + bundled fmt; nothing extra to build.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include/spdlog/spdlog.h")
    message(FATAL_ERROR
        "spdlog submodule missing. Run: git submodule update --init third_party/spdlog")
endif()
add_library(veyra_spdlog INTERFACE)
add_library(veyra::spdlog ALIAS veyra_spdlog)
target_include_directories(veyra_spdlog INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include")

# --- JUCE (submodule, GPL tier) --------------------------------------------
# Required by veyra-ui (Phase 4+). Bringing the subdirectory in only makes the
# JUCE targets available; nothing compiles unless veyra-ui links them.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE submodule missing. Run: git submodule update --init third_party/JUCE")
endif()
add_subdirectory(JUCE)

# --- Other submodule-backed dependencies (later phases) --------------------
if(NOT VEYRA_FETCH_DEPS)
    message(STATUS "third_party: optional submodule dependencies disabled (stub build)")
    return()
endif()

function(veyra_require_submodule name path)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${path}/CMakeLists.txt")
        message(FATAL_ERROR
            "Submodule '${name}' not found at third_party/${path}.\n"
            "Run: git submodule update --init --recursive third_party/${path}")
    endif()
endfunction()

# --- Logging (Phase 3+) ---
# veyra_require_submodule(spdlog spdlog)
# add_subdirectory(spdlog EXCLUDE_FROM_ALL)

# --- FFT (optional; JUCE FFT is the default) ---
# veyra_require_submodule(kissfft kissfft)
# add_subdirectory(kissfft EXCLUDE_FROM_ALL)

# --- IPC schema codegen (IPC backbone) ---
# veyra_require_submodule(flatbuffers flatbuffers)
# add_subdirectory(flatbuffers EXCLUDE_FROM_ALL)

# --- UI framework (Phase 4) ---
# veyra_require_submodule(JUCE JUCE)
# add_subdirectory(JUCE EXCLUDE_FROM_ALL)

# --- Mic noise suppression (Phase 6) ---
# veyra_require_submodule(rnnoise rnnoise)
# add_subdirectory(rnnoise EXCLUDE_FROM_ALL)

message(STATUS "third_party: dependency wiring active (uncomment blocks as phases land)")
