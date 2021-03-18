add_executable( precomp_bessel precomp_bessel.cpp )
target_link_libraries(
  precomp_bessel
  ifm
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  Threads::Threads
)
target_include_directories(
  precomp_bessel
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${JSON_INCLUDE_DIR}
  Boost::header
)
