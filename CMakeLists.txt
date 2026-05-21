cmake_minimum_required(VERSION 3.16)
project(Lab1_ProcessCreation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared include directory contains employee.h
include_directories(include)

# ---- Targets ----
add_executable(Main     src/main.cpp)
add_executable(Creator  src/creator.cpp)
add_executable(Reporter src/reporter.cpp)

# ---- Compiler warnings ----
if(MSVC)
    target_compile_options(Main     PRIVATE /W4 /WX)
    target_compile_options(Creator  PRIVATE /W4 /WX)
    target_compile_options(Reporter PRIVATE /W4 /WX)
endif()

# ---- Output all binaries to the same directory so Main can find Creator/Reporter ----
set_target_properties(Main Creator Reporter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
