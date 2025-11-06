cmake_minimum_required(VERSION 3.22)

add_library(msvcrt SHARED
    console.c
    ctype.c
    data.c
    dir.c
    environ.c
    errno.c
    exit.c
    file.c
    heap.c
    iob.c
    locale.c
    lock.c
    main.c
    math.c
    mathf.c
    mbcs.c
    misc.c
    onexit.c
    process.c
    scanf.c
    sincos.c
    string.c
    thread.c
    time.c
    undname.c
    wcs.c
    stubs.c
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set_target_properties(msvcrt PROPERTIES OUTPUT_NAME msvcrt)

target_compile_definitions(msvcrt
    PRIVATE
        _CRTIMP=
        _MSVCR_VER=90
        _NTSYSTEM_ # stub ntdll calls
        __WINE_PE_BUILD
)

target_compile_options(msvcrt
    PRIVATE
        -fno-builtin
        -fno-strict-aliasing
        -fno-omit-frame-pointer
        -Wno-deprecated-declarations
        -Wno-unused-parameter
        -Wno-missing-braces
        -Wno-ignored-qualifiers
)

target_include_directories(msvcrt
    PRIVATE
        ${PROJECT_SOURCE_DIR}/dlls/msvcrt
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/msvcrt
        ${PROJECT_SOURCE_DIR}/include/wine
)

target_link_libraries(msvcrt
    PRIVATE
        musl
        kernel32_import_lib
)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-resource-dir
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(CLANG_RESOURCE_DIR)
    target_include_directories(msvcrt
        PRIVATE
            ${CLANG_RESOURCE_DIR}/include
    )
    set(CLANG_BUILTINS_LIB "${CLANG_RESOURCE_DIR}/lib/linux/libclang_rt.builtins-i386.a")
    if(EXISTS "${CLANG_BUILTINS_LIB}")
        target_link_libraries(msvcrt PRIVATE "${CLANG_BUILTINS_LIB}")
    endif()
endif()

set(_toolchain_target ${CMAKE_C_COMPILER_TARGET})
if(NOT _toolchain_target)
    set(_toolchain_target i686-pc-windows-gnu)
endif()

execute_process(
    COMMAND ${CMAKE_C_COMPILER} --target=${_toolchain_target} -print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(LIBGCC_PATH AND EXISTS "${LIBGCC_PATH}")
    target_link_libraries(msvcrt PRIVATE "${LIBGCC_PATH}")
endif()
