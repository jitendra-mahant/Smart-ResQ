cmake_minimum_required(VERSION 3.16)
project(SmartResQ_Engine VERSION 1.0.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings
if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Core static library
add_library(smartresq_core
    src/Graph.cpp
    src/GraphAlgorithms.cpp
    src/EmergencyDispatcher.cpp
    src/RoadBlockageSimulator.cpp
    src/EngineApi.cpp
    src/Dijkstra.cpp
)

target_include_directories(smartresq_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main simulation engine CLI
add_executable(smartresq_engine
    src/main.cpp
)
target_link_libraries(smartresq_engine PRIVATE smartresq_core)

# Unit test binary
add_executable(smartresq_tests
    tests/test_graph.cpp
)
target_link_libraries(smartresq_tests PRIVATE smartresq_core)

