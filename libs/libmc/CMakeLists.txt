include_directories(
    "include"
    "${MACHINA_ROOT}/kernel/include")

add_definitions("-DNOFLOAT")

add_library(libmc STATIC
    "source/CopyMemory.S"
    "source/CopyMemory16.S"
    "source/CopyMemory64.S"
    "source/FillMemory.S"
    "source/FormatString.cc"
    "source/StringLength.c"
    "source/stub.c"
    )
set_target_properties(libmc PROPERTIES
    OUTPUT_NAME "mc"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "lib")
