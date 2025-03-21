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

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/${LEDMATRIX_FILES}/"
        "${CMAKE_BINARY_DIR}/assets/${LEDMATRIX_FILES}"
)