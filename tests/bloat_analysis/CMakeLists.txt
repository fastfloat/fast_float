add_executable(bloaty main.cpp a1.cpp a2.cpp a3.cpp a4.cpp a4.cpp a5.cpp a6.cpp a7.cpp a8.cpp a9.cpp a10.cpp)
target_link_libraries(bloaty PUBLIC fast_float)

add_executable(bloatyref main_ref.cpp)
target_link_libraries(bloatyref PUBLIC fast_float)