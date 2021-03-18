add_executable( fm2spec fm2spec.cpp )
target_link_libraries(
  fm2spec
  stamp
  smfp
  ifm
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  fm2spec
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
  ${SNDFILE_INCLUDE_DIR}
)

