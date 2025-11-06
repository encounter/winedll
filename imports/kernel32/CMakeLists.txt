cmake_minimum_required(VERSION 3.22)

# set(KERNEL32_SPEC ${PROJECT_SOURCE_DIR}/../wine/dlls/kernel32/kernel32.spec)
set(KERNEL32_DEF ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)
set(KERNEL32_LIB ${CMAKE_CURRENT_BINARY_DIR}/kernel32.lib)

# add_custom_command(
#     OUTPUT ${KERNEL32_DEF}
#     COMMAND winebuild --target=i386-windows --def -E ${KERNEL32_SPEC} -o ${KERNEL32_DEF}
#     DEPENDS ${KERNEL32_SPEC}
#     COMMENT "Generating kernel32.def from Wine spec"
#     VERBATIM
# )

add_custom_command(
    OUTPUT ${KERNEL32_LIB}
    COMMAND llvm-dlltool -m i386 -d ${KERNEL32_DEF} -l ${KERNEL32_LIB}
    DEPENDS ${KERNEL32_DEF}
    COMMENT "Generating kernel32 import library"
    VERBATIM
)

add_custom_target(kernel32_import ALL DEPENDS ${KERNEL32_LIB})

add_library(kernel32_import_lib STATIC IMPORTED GLOBAL)
set_target_properties(kernel32_import_lib PROPERTIES
    IMPORTED_LOCATION ${KERNEL32_LIB}
)

add_dependencies(kernel32_import_lib kernel32_import)
