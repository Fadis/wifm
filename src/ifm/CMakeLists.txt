add_library( ifm SHARED
  fft.cpp
  bessel.cpp
  load_monoral.cpp
  spectrum_image.cpp
  2op.cpp
  exp_match.cpp
)
target_link_libraries(
  ifm
  stamp
  smfp
  ${FFTW_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  PFFFT::PFFFT
  Threads::Threads
)
target_include_directories(
  ifm
  PRIVATE
  ${FFTW_INCLUDE_DIRS}
  ${SNDFILE_INCLUDE_DIR}
  ${PFFFT_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include
)

