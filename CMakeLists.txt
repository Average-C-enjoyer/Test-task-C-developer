cmake_minimum_required(VERSION 3.20)

project(EChat LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable (handle_data 
	"src/main.c"
)

target_include_directories(handle_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable (test_data 
	"src/test.c"
)

target_include_directories(test_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
