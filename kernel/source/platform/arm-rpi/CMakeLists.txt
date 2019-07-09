add_library(platform_arm_rpi STATIC "sync.S" "systimer.cc" "armtimer.cc" "loctimer.cc")
target_link_libraries(platform_arm_rpi platform_arm)
set_target_properties(platform_arm_rpi PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "")