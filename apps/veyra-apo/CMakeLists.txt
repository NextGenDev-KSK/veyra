# veyra-apo — the Audio Processing Object DLL loaded into audiodg.exe.
#
# Phase 2: a real output APO implementing the core COM interfaces directly
# against the base Windows SDK (no WDK), running the shared veyra::dsp chain and
# reading parameters from the service via shared memory. Registration into the
# audio stack is done by the driver INF (installer/driver/) + test-signing on
# the target machine; CI verifies it compiles and links.

add_library(veyra-apo SHARED
    dllmain.cpp
    VeyraApo.cpp
    VeyraMicApo.cpp
    veyra-apo.rc)

set_target_properties(veyra-apo PROPERTIES
    OUTPUT_NAME "veyra-apo"
    PREFIX "")   # produce veyra-apo.dll, not libveyra-apo.dll

# Only SharedMemory.obj is pulled from veyra-common (no logging/spdlog), so the
# DLL stays lean for the real-time audio engine.
target_link_libraries(veyra-apo PRIVATE veyra::common veyra::dsp veyra::ns)

# Export the COM entry points by undecorated name.
target_link_options(veyra-apo PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/VeyraApo.def")
