cmake_minimum_required(VERSION 3.16)
project(Lab2_ThreadCreation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Lab2 src/main.cpp)

if(MSVC)
    target_compile_options(Lab2 PRIVATE /W4 /WX)
endif()
