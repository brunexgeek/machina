include_directories(
    "include"
    "${MACHINA_ROOT}/libs/libmc/include")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_LIST_DIR}/kernel.ld")

add_subdirectory(source/platform/arm)
add_subdirectory(source/platform/arm-rpi)

add_executable(kernel
    "source/vfs.c"
    "source/ramfs.c"
    "source/procfs.c"
    "source/heap.cc"
    "source/uart.c"
    "source/pmm.cc"
    "source/timer.cc"
    "source/mailbox.c"
    "source/Font.cc"
    "source/Display.cc"
    "source/operator.cc"
    "source/Screen.cc"
    "source/System.cc"
    "source/Kernel.cc"
    "source/tamzen-10x20.c"
    "source/platform/arm/system.S")
target_link_libraries(kernel platform_arm libmc  platform_arm_rpi )
set_target_properties(kernel PROPERTIES
    OUTPUT_NAME "kernel.elf"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(kernel.img
    ${CMAKE_OBJCOPY} ${CMAKE_BINARY_DIR}/kernel.elf -O binary ${CMAKE_BINARY_DIR}/kernel.img
    DEPENDS kernel "./kernel.ld")