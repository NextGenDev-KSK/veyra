# veyra-overlay — the Sound Tracker HUD, a tiny standalone process.
#
# Phase 0: a GUI-subsystem (WIN32) executable whose WinMain returns 0. Final
# form (Phase 8): a layered, click-through, topmost window drawn with Direct2D
# rendering radar/edge HUDs from tracker data over the named-pipe channel —
# never a graphics-API hook, so it stays anti-cheat safe.

add_executable(veyra-overlay WIN32
    main.cpp)

target_link_libraries(veyra-overlay PRIVATE veyra::common)
