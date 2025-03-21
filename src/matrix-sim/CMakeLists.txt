set(MATRIX_SIM_NAME matrixsim)

project(${MATRIX_SIM_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MATRIX_SIM_SOURCE_DIR .)

#find_library(GUILIB_LIBRARY guilib)
set(GUILIB_LIBRARY guilib)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_executable(${MATRIX_SIM_NAME} 
    ${MATRIX_SIM_SOURCE_DIR}/main.cpp
)

target_include_directories(${MATRIX_SIM_NAME} PUBLIC
    ${GUILIB_INCLUDE_DIR}
)

target_link_libraries(${MATRIX_SIM_NAME}
    ${GUILIB_LIBRARY}
)

target_compile_options(${MATRIX_SIM_NAME} PUBLIC
    -g
)