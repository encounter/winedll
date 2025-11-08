cmake_minimum_required(VERSION 3.22)

set(ADVAPI32_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.spec)
set(ADVAPI32_DEF ${CMAKE_CURRENT_BINARY_DIR}/advapi32.def)
set(ADVAPI32_LIB ${CMAKE_CURRENT_BINARY_DIR}/advapi32.lib)

add_custom_command(
    OUTPUT ${ADVAPI32_DEF}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_DEF_TOOL}
        --arch i386
        --imports-only
        ${ADVAPI32_SPEC}
        ${ADVAPI32_DEF}
    DEPENDS
        ${ADVAPI32_SPEC}
        ${GENERATE_DEF_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating advapi32.def"
    VERBATIM
)

add_custom_command(
    OUTPUT ${ADVAPI32_LIB}
    COMMAND llvm-dlltool -m i386 -k -d ${ADVAPI32_DEF} -l ${ADVAPI32_LIB}
    DEPENDS ${ADVAPI32_DEF}
    COMMENT "Generating advapi32 import library"
    VERBATIM
)

add_custom_target(advapi32_import ALL DEPENDS ${ADVAPI32_LIB})

add_library(advapi32_import_lib STATIC IMPORTED GLOBAL)
set_target_properties(advapi32_import_lib PROPERTIES
    IMPORTED_LOCATION ${ADVAPI32_LIB}
)

add_dependencies(advapi32_import_lib advapi32_import)
