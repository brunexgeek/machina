file(GLOB SOURCE
    "source/*.c"
    "source/*.S")

include_directories(
    "include"
    "${MACHINA_ROOT}/kernel/include")

add_definitions("-DNOFLOAT")

add_library(libmc STATIC ${SOURCE})
set_target_properties(libmc PROPERTIES
    OUTPUT_NAME "mc"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "lib")
