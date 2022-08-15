cmake_minimum_required(VERSION 3.23)
project(task_manager)

set(CMAKE_CXX_STANDARD 23)

set(AMALGAMATED ${PROJECT_NAME}_a)
set(NAMALGAMATED ${PROJECT_NAME})

add_executable(${NAMALGAMATED}
        main.cpp
        src/task_manager.h src/task_manager.cpp
        src/util.h)

add_executable(${AMALGAMATED}
        main.cpp
        release/task_manager.hpp)

target_compile_definitions(${AMALGAMATED} PUBLIC AMALGAMATED)
