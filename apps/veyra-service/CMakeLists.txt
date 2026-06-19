# veyra-service — the orchestrator Windows service.
#
# Phase 1: a console exe that runs as an SCM service (or --console for
# debugging), hosting the named-pipe control server and the config manager.
# Later phases add per-app/device rules, loopback capture, the Sound Tracker
# engine, the updater and crash collection. The app manifest is embedded via a
# .rc; the service is registered to run as LocalSystem at install time.

add_executable(veyra-service
    main.cpp
    ServiceMain.cpp
    ServiceInstaller.cpp
    ServiceRuntime.cpp
    ConfigManager.cpp
    ControlServer.cpp
    PresetLibrary.cpp
    ApoPublisher.cpp
    MicPublisher.cpp
    TrackerService.cpp
    SleepTimerService.cpp
    AudioBridge.cpp
    veyra-service.rc)

# advapi32: SCM APIs; veyra::dsp: the Sound Tracker engine; ole32: WASAPI/COM.
target_link_libraries(veyra-service PRIVATE veyra::common veyra::dsp advapi32 ole32)

# Ensure the resource compiler can locate veyra-service.exe.manifest referenced
# from veyra-service.rc (CMake forwards include dirs to rc.exe as /I).
target_include_directories(veyra-service PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# We embed our own RT_MANIFEST via veyra-service.rc, so suppress the linker's
# default manifest to avoid an mt.exe duplicate-manifest conflict.
if(MSVC)
    target_link_options(veyra-service PRIVATE "/MANIFEST:NO")
endif()
