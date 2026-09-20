cmake_minimum_required(VERSION 3.15...3.25)

project(convert2pgn LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(convert2pgn
  src/main.cpp
  src/parsor.cpp 
)

target_include_directories(convert2pgn PRIVATE include include/convert2pgn)
