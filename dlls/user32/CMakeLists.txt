cmake_minimum_required(VERSION 3.22)

set(USER32_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/user32.spec)
set(USER32_DEF ${CMAKE_CURRENT_BINARY_DIR}/user32.def)
set(USER32_LIB ${CMAKE_CURRENT_BINARY_DIR}/user32.lib)

add_custom_command(
    OUTPUT ${USER32_DEF}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_DEF_TOOL}
        --arch i386
        --imports-only
        ${USER32_SPEC}
        ${USER32_DEF}
    DEPENDS
        ${USER32_SPEC}
        ${GENERATE_DEF_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating user32.def"
    VERBATIM
)

add_custom_command(
    OUTPUT ${USER32_LIB}
    COMMAND llvm-dlltool -m i386 -k -d ${USER32_DEF} -l ${USER32_LIB}
    DEPENDS ${USER32_DEF}
    COMMENT "Generating user32 import library"
    VERBATIM
)

add_custom_target(user32_import ALL DEPENDS ${USER32_LIB})

add_library(user32_import_lib STATIC IMPORTED GLOBAL)
set_target_properties(user32_import_lib PROPERTIES
    IMPORTED_LOCATION ${USER32_LIB}
)

add_dependencies(user32_import_lib user32_import)
