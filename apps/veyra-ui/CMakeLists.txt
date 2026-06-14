# veyra-ui — the main user-facing application. Produces veyra.exe.
#
# Phase 1: a minimal native Win32 shell that displays the live service
# connection status via the background ServiceClient (named-pipe IPC). No JUCE
# yet — that replaces this shell in Phase 4. The manifest (asInvoker +
# per-monitor-v2 DPI awareness) is already embedded for correct HiDPI behaviour.

add_executable(veyra WIN32
    Source/Main.cpp
    Source/ServiceClient.cpp
    veyra.rc)

target_include_directories(veyra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(veyra PRIVATE veyra::common veyra::dsp user32 gdi32)

# Ensure the resource compiler can locate veyra.exe.manifest referenced from
# veyra.rc (CMake forwards include dirs to rc.exe as /I).
target_include_directories(veyra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# We embed our own RT_MANIFEST via veyra.rc, so suppress the linker's default
# manifest to avoid an mt.exe duplicate-manifest conflict.
if(MSVC)
    target_link_options(veyra PRIVATE "/MANIFEST:NO")
endif()
