cmake_minimum_required(VERSION 4.3)
project(LTDT_26_27_PathGo)

set(CMAKE_CXX_STANDARD 14)

include_directories(workflows/app)

add_executable(LTDT_26_27_PathGo
        workflows/app/main.cpp)
