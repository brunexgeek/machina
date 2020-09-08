add_library(bcm2837 STATIC
    "entrypoint.S"
    "crt.S")
set_target_properties(bcm2837 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "")