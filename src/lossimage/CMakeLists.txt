add_executable( lossimage lossimage.cpp )
target_link_libraries(
  lossimage
  stamp
  smfp
  ifm
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  lossimage
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
  ${SNDFILE_INCLUDE_DIR}
)



