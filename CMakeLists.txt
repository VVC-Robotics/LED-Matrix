cmake_minimum_required(VERSION 3.10)

set(LEDMATRIX_NAME LED-Matrix-2)

project(${LEDMATRIX_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LEDMATRIX_FILES models)
set(LEDMATRIX_SOURCE_DIR src)

add_subdirectory(${LEDMATRIX_SOURCE_DIR}/guilib)
add_subdirectory(${LEDMATRIX_SOURCE_DIR}/matrix-sim)

add_definitions(CMAKE_EXPORT_COMPILE_COMMANDS=ON)