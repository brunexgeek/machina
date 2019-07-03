#
# 'system.S' in used directly by the 'kernel'
#

add_library(platform_arm STATIC "idiv.S" "uidivmod.S")
set_target_properties(platform_arm PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "")