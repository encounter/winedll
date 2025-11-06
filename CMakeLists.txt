cmake_minimum_required(VERSION 3.22)

project(winedll LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(libs/musl)
add_subdirectory(imports/kernel32)
add_subdirectory(dlls/msvcrt)
