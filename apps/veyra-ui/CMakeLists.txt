# veyra-ui — the main JUCE application. Produces veyra.exe.
#
# Phase 4a: JUCE integration + theme engine + glass renderer + component
# library, shown via a Foundations/Gallery view so the design language can be
# verified by running the CI-built artifact. Home / Mini-mode / Settings land
# in 4b/4c.

juce_add_gui_app(veyra
    PRODUCT_NAME "Veyra Sounds"
    COMPANY_NAME "Veyra Sounds"
    BUNDLE_ID "com.veyra.sounds")

# Embed fonts (OFL). BinaryData::Orbitron_ttf / Inter_ttf / JetBrainsMono_ttf.
juce_add_binary_data(veyra_assets SOURCES
    "${CMAKE_SOURCE_DIR}/resources/fonts/Orbitron.ttf"
    "${CMAKE_SOURCE_DIR}/resources/fonts/Inter.ttf"
    "${CMAKE_SOURCE_DIR}/resources/fonts/JetBrainsMono.ttf")

target_sources(veyra PRIVATE
    Source/Main.cpp
    Source/RootComponent.cpp
    Source/Theme/DesignTokens.cpp
    Source/Theme/Fonts.cpp
    Source/Theme/VeyraLookAndFeel.cpp
    Source/Graphics/GlassBackground.cpp
    Source/Components/ToggleSwitch.cpp
    Source/Components/Knob.cpp
    Source/Components/SegmentedControl.cpp
    Source/Components/EqBandSlider.cpp
    Source/Home/TopBar.cpp
    Source/Home/Sidebar.cpp
    Source/Home/VisualizerCard.cpp
    Source/Home/EqualizerCard.cpp
    Source/Screens/HomeScreen.cpp)

target_include_directories(veyra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(veyra PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="Veyra Sounds"
    JUCE_APPLICATION_VERSION_STRING="${VEYRA_VERSION}")

target_link_libraries(veyra PRIVATE
    veyra_assets
    veyra::common
    veyra::dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# Keep the binary in the unified bin/ dir as veyra.exe (matches CI checks).
set_target_properties(veyra PROPERTIES
    OUTPUT_NAME "veyra"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
