# veyra-service — the orchestrator Windows service.
#
# Phase 0: a console executable whose main() returns 0. Real responsibilities
# (named-pipe control server, config/preset state, per-app/per-device rules,
# loopback capture, Sound Tracker engine, updater, crash collector) arrive from
# Phase 1 onward. The app manifest is embedded via a .rc so it is present from
# day one; the service is registered to run as LocalSystem at install time, not
# via the manifest.

add_executable(veyra-service
    main.cpp
    veyra-service.rc)

target_link_libraries(veyra-service PRIVATE veyra::common)
