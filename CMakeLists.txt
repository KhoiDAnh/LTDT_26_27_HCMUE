cmake_minimum_required(VERSION 4.3)
project(LTDT_26_27_PathGo)

set(CMAKE_CXX_STANDARD 14)

include_directories(Source/app)

add_executable(LTDT_26_27_PathGo
        Source/app/main.cpp
        Source/include/Maze.h
        Source/include/Graph.h
        Source/include/Dijkstra.h
        Source/include/Visualization.h
        Source/src/Maze.cpp
        Source/src/Graph.cpp
        Source/src/Dijkstra.cpp
        Source/src/Visualization.cpp)
