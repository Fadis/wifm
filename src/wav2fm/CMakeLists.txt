add_executable( wav2fm wav2fm.cpp )
target_link_libraries(
  wav2fm
  stamp
  smfp
  ifm
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  wav2fm
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
  ${SNDFILE_INCLUDE_DIR}
)


