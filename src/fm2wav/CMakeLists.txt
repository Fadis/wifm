add_executable( fm2wav fm2wav.cpp )
target_link_libraries(
  fm2wav
  stamp
  smfp
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  fm2wav
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
  ${SNDFILE_INCLUDE_DIR}
)

