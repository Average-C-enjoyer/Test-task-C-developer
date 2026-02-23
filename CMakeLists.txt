cmake_minimum_required(VERSION 3.20)

project(EChat LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable (handle_binary_data 
	"src/test_task.c"
)

target_include_directories(handle_binary_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
