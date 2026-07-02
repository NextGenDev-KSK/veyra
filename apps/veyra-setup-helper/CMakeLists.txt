# veyra-setup-helper — Native installer helper executable
#
# Called silently by the NSIS installer (nsExec::ExecToLog) to perform
# operations that require Windows audio APIs:
#   - Enumerate active render endpoints (IMMDeviceEnumerator)
#   - Associate / remove the Veyra APO from endpoints (registry write)
#   - Restart Windows Audio service (SCM API)
#   - Verify COM registration
#
# Compiled with /MT (static CRT) so it has no VC++ runtime dependency —
# this exe runs on bare Windows 10/11 before any redistributable is installed.
#
# No WDK required — only the standard Windows SDK.

add_executable(veyra-setup-helper
    main.cpp
    veyra-setup-helper.rc)

# Static CRT so the helper has zero runtime dependencies at install time
set_target_properties(veyra-setup-helper PROPERTIES
    OUTPUT_NAME "VeyraSetupHelper"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_features(veyra-setup-helper PRIVATE cxx_std_17)

target_link_libraries(veyra-setup-helper PRIVATE
    ole32       # COM: CoCreateInstance, CoInitializeEx, CoTaskMemFree
    propsys     # IPropertyStore, PropVariantClear
    uuid        # IID_*/CLSID_* GUIDs
    advapi32    # Registry APIs, SCM APIs
)

# Silence MSVC's "use _s variants" warnings — we use _s variants already.
target_compile_definitions(veyra-setup-helper PRIVATE
    _CRT_SECURE_NO_WARNINGS)
