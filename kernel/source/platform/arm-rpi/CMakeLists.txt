add_library(platform_arm_rpi STATIC "sync.S" "systimer.cc" "armtimer.cc" "loctimer.cc")
set_target_properties(platform_arm_rpi PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "")