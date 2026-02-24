add_library(fast_float_strtod STATIC fast_float_strtod.cpp)
target_link_libraries(fast_float_strtod PRIVATE fast_float)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
    target_link_options(fast_float_strtod PRIVATE -nostdlib++)
endif()

add_library(FastFloat::fast_float_strtod ALIAS fast_float_strtod)
target_include_directories(
  fast_float_strtod
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)