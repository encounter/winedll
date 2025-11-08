cmake_minimum_required(VERSION 3.22)

set(KERNELBASE_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/kernelbase.spec)
set(KERNELBASE_DEF ${CMAKE_CURRENT_BINARY_DIR}/kernelbase.def)
set(KERNELBASE_LIB ${CMAKE_CURRENT_BINARY_DIR}/kernelbase.lib)

add_custom_command(
    OUTPUT ${KERNELBASE_DEF}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_DEF_TOOL}
        --arch i386
        --imports-only
        ${KERNELBASE_SPEC}
        ${KERNELBASE_DEF}
    DEPENDS
        ${KERNELBASE_SPEC}
        ${GENERATE_DEF_TOOL}
        ${SPEC_PARSER_LIB}
    COMMENT "Generating kernelbase.def"
    VERBATIM
)

add_custom_command(
    OUTPUT ${KERNELBASE_LIB}
    COMMAND llvm-dlltool -m i386 -k -d ${KERNELBASE_DEF} -l ${KERNELBASE_LIB}
    DEPENDS ${KERNELBASE_DEF}
    COMMENT "Generating kernelbase import library"
    VERBATIM
)

add_custom_target(kernelbase_import ALL DEPENDS ${KERNELBASE_LIB})

add_library(kernelbase_import_lib STATIC IMPORTED GLOBAL)
set_target_properties(kernelbase_import_lib PROPERTIES
    IMPORTED_LOCATION ${KERNELBASE_LIB}
)

add_dependencies(kernelbase_import_lib kernelbase_import)
