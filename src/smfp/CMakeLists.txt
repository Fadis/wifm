add_library( smfp SHARED
  get_volume.cpp
  get_frequency.cpp
  dummy_handler.cpp
  channel_state.cpp
  global_state.cpp
  envelope_generator.cpp
  fm.cpp
  2op.cpp
  get_node.cpp
  mixer.cpp
  wavesink.cpp
)
target_link_libraries(
  smfp
  stamp
  ${SNDFILE_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  smfp
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

