# Test suite. Built only when VEYRA_BUILD_TESTS=ON (CI passes this). Catch2 v2
# is vendored as veyra::catch2; veyra::dsp is header-only so tests link straight
# against it.

add_executable(dsp_tests
    dsp_tests/main.cpp
    dsp_tests/test_core.cpp
    dsp_tests/test_eq_stereo.cpp
    dsp_tests/test_dynamics.cpp
    dsp_tests/test_analyzer.cpp
    dsp_tests/test_chain.cpp
    dsp_tests/test_shared_params.cpp
    dsp_tests/test_voice.cpp
    dsp_tests/test_spatial.cpp
    dsp_tests/test_virtual_surround.cpp
    dsp_tests/test_tracker.cpp
    dsp_tests/test_lab_loudness.cpp
    dsp_tests/test_loudness.cpp
    dsp_tests/test_truepeak_norm.cpp
    dsp_tests/test_delayline.cpp
    dsp_tests/test_sleep_timer.cpp
    dsp_tests/test_output_router.cpp
    dsp_tests/test_performance.cpp
    dsp_tests/test_reverb.cpp
    dsp_tests/test_parametric_eq.cpp
    dsp_tests/test_chain_parametric.cpp
    dsp_tests/test_agc.cpp
    dsp_tests/test_aec.cpp
    dsp_tests/test_scene_detector.cpp
    dsp_tests/test_hrtf_kemar.cpp)

target_include_directories(dsp_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dsp_tests")
# veyra::common for the header-only shared-parameter seqlock contract.
target_link_libraries(dsp_tests PRIVATE veyra::dsp veyra::common veyra::catch2)
# Point the HRTF tests at the in-repo MIT KEMAR diffuse set.
target_compile_definitions(dsp_tests PRIVATE
    VEYRA_KEMAR_DIR="${CMAKE_SOURCE_DIR}/third_party/hrtf/mit_kemar/diffuse")

add_test(NAME dsp_tests COMMAND dsp_tests)

# Phase 5: presets, per-app rules + rate limiter, per-device profiles.
add_executable(common_tests
    dsp_tests/main.cpp
    common_tests/test_presets.cpp
    common_tests/test_app_rules.cpp
    common_tests/test_device_profiles.cpp
    common_tests/test_voice_config.cpp
    common_tests/test_tracker_data.cpp
    common_tests/test_spatial_config.cpp
    common_tests/test_gamer_config.cpp
    common_tests/test_loudness_config.cpp
    common_tests/test_sharing_config.cpp
    common_tests/test_appearance_config.cpp
    common_tests/test_analyzer_data.cpp
    common_tests/test_bridge_config.cpp
    common_tests/test_localization.cpp
    common_tests/test_updater.cpp
    common_tests/test_crash_report.cpp
    common_tests/test_hotkeys.cpp
    common_tests/test_game_detector.cpp
    common_tests/test_rnnoise.cpp
    common_tests/test_autoeq.cpp)

target_link_libraries(common_tests PRIVATE veyra::common veyra::catch2 veyra::ns)
target_compile_definitions(common_tests PRIVATE
    VEYRA_AUTOEQ_DIR="${CMAKE_SOURCE_DIR}/resources/autoeq")

add_test(NAME common_tests COMMAND common_tests)
