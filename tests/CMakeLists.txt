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
    dsp_tests/test_shared_params.cpp)

target_include_directories(dsp_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dsp_tests")
# veyra::common for the header-only shared-parameter seqlock contract.
target_link_libraries(dsp_tests PRIVATE veyra::dsp veyra::common veyra::catch2)

add_test(NAME dsp_tests COMMAND dsp_tests)
