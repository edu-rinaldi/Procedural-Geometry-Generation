add_executable(yscenegen  yscenegen.cpp ext/perlin-noise/noise1234.cpp)

set_target_properties(yscenegen  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(yscenegen  PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(yscenegen  yocto)

