# veyra-common — shared utilities used by every target: version info, paths,
# logging, config (JSON), and the named-pipe IPC transport + protocol.

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/veyra/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/veyra/version.h"
    @ONLY)

add_library(veyra-common STATIC
    src/version.cpp
    src/Paths.cpp
    src/Logging.cpp
    src/Config.cpp
    src/Preset.cpp
    src/AppRules.cpp
    src/DeviceProfiles.cpp
    src/Localization.cpp
    src/Updater.cpp
    src/ipc/PipeIo.cpp
    src/ipc/PipeServer.cpp
    src/ipc/PipeClient.cpp
    src/ipc/SharedMemory.cpp)

add_library(veyra::common ALIAS veyra-common)

target_include_directories(veyra-common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_features(veyra-common PUBLIC cxx_std_20)

# JSON and spdlog are implementation details (used only inside Config.cpp /
# Logging.cpp), so keep them PRIVATE — consumers of veyra::common don't inherit
# their include paths or compile cost.
target_link_libraries(veyra-common
    PRIVATE veyra::json veyra::spdlog
    PUBLIC  shell32 ole32)   # SHGetKnownFolderPath / CoTaskMemFree
