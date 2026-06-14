# veyra-ui — the main user-facing JUCE application. Produces veyra.exe.
#
# Phase 0: a GUI-subsystem (WIN32) executable whose WinMain returns 0 — no
# JUCE yet, so the CI build is fast. The manifest (asInvoker + per-monitor-v2
# DPI awareness) is embedded now so HiDPI behaviour is correct the moment real
# UI lands in Phase 4.

add_executable(veyra WIN32
    Source/Main.cpp
    veyra.rc)

target_link_libraries(veyra PRIVATE veyra::common veyra::dsp)
