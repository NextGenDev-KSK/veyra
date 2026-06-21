# veyra-rnnoise — opaque RNNoise mic-denoiser wrapper.
#
# Always builds (so callers can link it unconditionally); when RNNoise itself is
# available (veyra::rnnoise, set by third_party/), it links it and defines
# VEYRA_USE_RNNOISE=1 PUBLICly so consumers/tests can branch. Otherwise the
# wrapper compiles to a pass-through stub.

add_library(veyra_rnnoise_wrap STATIC src/RnnoiseDenoiser.cpp)
add_library(veyra::ns ALIAS veyra_rnnoise_wrap)

target_include_directories(veyra_rnnoise_wrap PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(VEYRA_HAVE_RNNOISE)
    target_link_libraries(veyra_rnnoise_wrap PRIVATE veyra::rnnoise)
    target_compile_definitions(veyra_rnnoise_wrap PUBLIC VEYRA_USE_RNNOISE=1)
endif()
