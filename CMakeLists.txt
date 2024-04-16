cmake_minimum_required(VERSION 3.14)

project(bc VERSION 1.0
           LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

add_subdirectory(examples)

add_executable(bc main.cpp)

target_link_libraries(bc fmt log async)
