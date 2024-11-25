add_executable(realbenchmark benchmark.cpp)
set_property(
    TARGET realbenchmark
    PROPERTY CXX_STANDARD 17)

target_link_libraries(realbenchmark PUBLIC fast_float)
include(ExternalProject)

# Define the external project
ExternalProject_Add(simple_fastfloat_benchmark
    GIT_REPOSITORY https://github.com/lemire/simple_fastfloat_benchmark.git
    GIT_TAG        master  # or specify a particular commit/tag/branch
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/simple_fastfloat_benchmark
    BINARY_DIR     ${CMAKE_BINARY_DIR}/simple_fastfloat_benchmark-build
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
set(DATA_DIR ${CMAKE_BINARY_DIR}/simple_fastfloat_benchmark/data)

add_custom_target(CopyData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_DIR} ${CMAKE_CURRENT_BINARY_DIR}/data
    DEPENDS simple_fastfloat_benchmark
)
add_dependencies(realbenchmark CopyData)
target_compile_definitions(realbenchmark PUBLIC BENCHMARK_DATA_DIR="${CMAKE_CURRENT_BINARY_DIR}/data")
