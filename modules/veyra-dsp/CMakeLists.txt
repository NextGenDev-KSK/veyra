# veyra-dsp — shared DSP library, header-only where possible.
#
# Phase 0: an INTERFACE target exposing the include/ tree (eq/, dynamics/,
# spatial/, voice/, ... — currently empty placeholders). Real EQ, dynamics,
# spatial, voice, loudness and analyzer code is added in Phases 3, 6 and 7.

add_library(veyra-dsp INTERFACE)
add_library(veyra::dsp ALIAS veyra-dsp)

target_include_directories(veyra-dsp INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_features(veyra-dsp INTERFACE cxx_std_20)
