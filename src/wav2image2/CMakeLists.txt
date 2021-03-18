add_executable( wav2image2 wav2image2.cpp )
target_link_libraries(
  wav2image2
  stamp
  smfp
  ifm
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${FFTW_LIBRARIES}
  ${OIIO_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  wav2image2
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
  ${SNDFILE_INCLUDE_DIR}
  ${FFTW_INCLUDE_DIR}
  ${OIIO_INCLUDE_DIR}
)



