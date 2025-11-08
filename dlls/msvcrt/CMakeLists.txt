cmake_minimum_required(VERSION 3.22)

file(GLOB MSVCR_SPEC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../msvcr*/msvcr*.spec)
set(UCRTBASE_SOURCE_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/../ucrtbase/ucrtbase.spec)

# Generated files in build directory
set(MSVCRT_MERGED_SPEC ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.spec)
set(MSVCRT_GENERATED_STUBS ${CMAKE_CURRENT_BINARY_DIR}/generated_stubs.c)
set(MSVCRT_DEF ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.def)

# Merge spec files
add_custom_command(
    OUTPUT ${MSVCRT_MERGED_SPEC}
    COMMAND ${Python3_EXECUTABLE} ${MERGE_SPECS_TOOL}
        ${MSVCR_SPEC_FILES}
        ${UCRTBASE_SOURCE_SPEC}
        ${MSVCRT_MERGED_SPEC}
    DEPENDS
        ${MSVCR_SPEC_FILES}
        ${UCRTBASE_SOURCE_SPEC}
        ${MERGE_SPECS_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Merging spec files"
    VERBATIM
)

# Generate stub implementations from merged spec
add_custom_command(
    OUTPUT ${MSVCRT_GENERATED_STUBS}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_STUBS_TOOL}
        --arch i386
        ${MSVCRT_MERGED_SPEC}
        ${MSVCRT_GENERATED_STUBS}
    DEPENDS
        ${MSVCRT_MERGED_SPEC}
        ${GENERATE_STUBS_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating stubs"
    VERBATIM
)

# Generate .def file from merged spec
add_custom_command(
    OUTPUT ${MSVCRT_DEF}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_DEF_TOOL}
        --arch i386
        --no-stdcall-suffix
        ${MSVCRT_MERGED_SPEC}
        ${MSVCRT_DEF}
    DEPENDS
        ${MSVCRT_MERGED_SPEC}
        ${GENERATE_DEF_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating msvcrt.def"
    VERBATIM
)

add_library(msvcrt SHARED
    concurrency.c
    console.c
    cpp.c
    # crt_gccmain.c
    # crt_main.c
    # crt_winmain.c
    # crt_wmain.c
    # crt_wwinmain.c
    ctype.c
    data.c
    dir.c
    environ.c
    errno.c
    except.c
    except_i386.c
    exception_ptr.c
    exit.c
    file.c
    handler4.c
    heap.c
    # iob.c
    locale.c
    lock.c
    main.c
    math.c
    mathf.c
    mbcs.c
    misc.c
    # onexit.c
    printf.c
    process.c
    scanf.c
    sincos.c
    string.c
    thread.c
    time.c
    undname.c
    wcs.c
    stubs.c
    ${MSVCRT_GENERATED_STUBS}
    ../winecrt0/crt_dllmain.c
    ../winecrt0/setjmp.c
    ../winecrt0/exception.c
    ${MSVCRT_DEF}
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set_target_properties(msvcrt PROPERTIES OUTPUT_NAME msvcrt)

target_compile_definitions(msvcrt
    PRIVATE
        _CRTIMP=
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

target_link_options(msvcrt
    PRIVATE
        -Wl,--enable-stdcall-fixup
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
        advapi32_import_lib
        kernel32_import_lib
        kernelbase_import_lib
        user32_import_lib
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_link_libraries(msvcrt
        PRIVATE
            ${CMAKE_SOURCE_DIR}/vendor/libclang_rt.builtins-i386.a
    )
endif()
