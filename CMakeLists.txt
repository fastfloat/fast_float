cmake_minimum_required(VERSION 3.9)

project(fast_float VERSION 0.7.0 LANGUAGES CXX)
option(FASTFLOAT_TEST "Enable tests" OFF)
if(FASTFLOAT_TEST)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED OFF)
  enable_testing()
  add_subdirectory(tests)
else(FASTFLOAT_TEST)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  message(STATUS "Tests are disabled. Set FASTFLOAT_TEST to ON to run tests.")
endif(FASTFLOAT_TEST)

option(FASTFLOAT_SANITIZE "Sanitize addresses" OFF)

if (NOT CMAKE_BUILD_TYPE)
  if(FASTFLOAT_SANITIZE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  else()
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()


add_library(fast_float INTERFACE)
target_include_directories(
  fast_float
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
if(FASTFLOAT_SANITIZE)
  target_compile_options(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(fast_float INTERFACE -fuse-ld=gold)
  endif()
endif()

include(CMakePackageConfigHelpers)

set(FASTFLOAT_VERSION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/fast_float-config-version.cmake")
set(FASTFLOAT_PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/fast_float-config.cmake")
set(FASTFLOAT_INSTALL_DIR "share/fast_float")

write_basic_package_version_file("${FASTFLOAT_VERSION_CONFIG}" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
configure_package_config_file("cmake/config.cmake.in"
                              "${FASTFLOAT_PROJECT_CONFIG}"
                              INSTALL_DESTINATION "${FASTFLOAT_INSTALL_DIR}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/fast_float" DESTINATION "include")
install(FILES "${FASTFLOAT_PROJECT_CONFIG}" "${FASTFLOAT_VERSION_CONFIG}" DESTINATION "${FASTFLOAT_INSTALL_DIR}")
install(EXPORT fast_float-targets NAMESPACE fast_float:: DESTINATION "${FASTFLOAT_INSTALL_DIR}")

install(TARGETS fast_float 
        EXPORT fast_float-targets
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)