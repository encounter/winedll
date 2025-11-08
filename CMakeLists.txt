cmake_minimum_required(VERSION 3.22)

project(winedll LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find Python for build scripts
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Define paths to build tools
set(MERGE_SPECS_TOOL ${CMAKE_SOURCE_DIR}/tools/merge_specs.py)
set(GENERATE_DEF_TOOL ${CMAKE_SOURCE_DIR}/tools/generate_def.py)
set(GENERATE_STUBS_TOOL ${CMAKE_SOURCE_DIR}/tools/generate_stubs.py)
set(SPEC_PARSER_LIB ${CMAKE_SOURCE_DIR}/tools/spec_parser.py)

add_subdirectory(dlls/advapi32)
add_subdirectory(dlls/kernel32)
add_subdirectory(dlls/kernelbase)
add_subdirectory(dlls/msvcrt)
add_subdirectory(dlls/user32)
add_subdirectory(libs/musl)
