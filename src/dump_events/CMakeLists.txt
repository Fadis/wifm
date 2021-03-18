add_executable( dump_events dump_events.cpp )
target_link_libraries(
  dump_events
  stamp
  smfp
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  dump_events
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  Boost::header
  ${JSON_INCLUDE_DIR}
)

