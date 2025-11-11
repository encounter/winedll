cmake_minimum_required(VERSION 3.22)

set(NTDLL_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/ntdll.spec)
set(NTDLL_DEF ${CMAKE_CURRENT_BINARY_DIR}/ntdll.def)
set(NTDLL_LIB ${CMAKE_CURRENT_BINARY_DIR}/ntdll.lib)

add_custom_command(
    OUTPUT ${NTDLL_DEF}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_DEF_TOOL}
        --arch i386
        --imports-only
        ${NTDLL_SPEC}
        ${NTDLL_DEF}
    DEPENDS
        ${NTDLL_SPEC}
        ${GENERATE_DEF_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating ntdll.def"
    VERBATIM
)

add_custom_command(
    OUTPUT ${NTDLL_LIB}
    COMMAND llvm-dlltool -m i386 -k -d ${NTDLL_DEF} -l ${NTDLL_LIB}
    DEPENDS ${NTDLL_DEF}
    COMMENT "Generating ntdll import library"
    VERBATIM
)

add_custom_target(ntdll_import ALL DEPENDS ${NTDLL_LIB})

add_library(ntdll_import_lib STATIC IMPORTED GLOBAL)
set_target_properties(ntdll_import_lib PROPERTIES
    IMPORTED_LOCATION ${NTDLL_LIB}
)

add_dependencies(ntdll_import_lib ntdll_import)
