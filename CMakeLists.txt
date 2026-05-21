cmake_minimum_required(VERSION 3.16)
project(Lab3_ThreadSync CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Lab3 src/main.cpp)

if(MSVC)
    target_compile_options(Lab3 PRIVATE /W4 /WX)
endif()
