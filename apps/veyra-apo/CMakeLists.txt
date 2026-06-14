# veyra-apo — the Audio Processing Object DLL loaded into audiodg.exe.
#
# Phase 0: a no-op DLL whose DllMain returns TRUE. It deliberately does NOT
# include the APO/WDK SDK (audioenginebaseapo.h) or implement any COM
# interfaces — that arrives in Phase 2 once the real-time DSP contract and
# shared-memory parameter block are designed. Keeping it SDK-free means the
# CI build needs only the base Windows SDK, not the WDK.

add_library(veyra-apo SHARED
    dllmain.cpp)

set_target_properties(veyra-apo PROPERTIES
    OUTPUT_NAME "veyra-apo"
    PREFIX "")   # produce veyra-apo.dll, not libveyra-apo.dll

target_link_libraries(veyra-apo PRIVATE veyra::common veyra::dsp)
