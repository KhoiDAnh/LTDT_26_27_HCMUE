# CMakeLists.txt (repo root)
cmake_minimum_required(VERSION 3.16)
project(LTDT_26_27_PathGo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect every .cpp under Source/src, re-checked on each build.
file(GLOB_RECURSE PATHGO_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/src/*.cpp)

# The shared "kitchen": all modules compiled once.
add_library(pathgo_core STATIC ${PATHGO_SOURCES})
target_include_directories(pathgo_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/include)

# The program: only main.cpp, plus the kitchen.
add_executable(LTDT_26_27_PathGo Source/app/main.cpp)
target_link_libraries(LTDT_26_27_PathGo PRIVATE pathgo_core)