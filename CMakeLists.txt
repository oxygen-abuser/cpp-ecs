cmake_minimum_required(VERSION 4.3.1)

project(engine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_subdirectory(src)

add_custom_target(filter-compdb1 ALL
    COMMAND python3 filter_compdb.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Filtering C files from _deps in compile_commands.json"
)

add_compile_definitions(WL_EGL_PLATFORM=1)
