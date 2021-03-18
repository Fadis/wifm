add_executable( wasserstein wasserstein.cpp )
target_link_libraries(
  wasserstein
  Threads::Threads
)
target_include_directories(
  wasserstein
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
