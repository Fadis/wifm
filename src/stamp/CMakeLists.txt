add_library( stamp SHARED
  mapped_file.cpp
)
target_link_libraries(
  stamp
  Threads::Threads
)
target_include_directories(
  stamp
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

