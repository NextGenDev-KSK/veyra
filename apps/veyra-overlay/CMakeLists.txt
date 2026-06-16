# veyra-overlay — the Sound Tracker HUD, a tiny standalone process.
#
# A layered, click-through, topmost window drawn with GDI+, rendering the radar
# from the VeyraTrackerData shared block written by the service's loopback Sound
# Tracker. It is an ordinary composited top-level window — never a graphics-API
# hook or injection — so it stays anti-cheat safe.

add_executable(veyra-overlay WIN32
    main.cpp)

target_link_libraries(veyra-overlay PRIVATE veyra::common gdiplus)
