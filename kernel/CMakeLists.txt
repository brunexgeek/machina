include_directories(
    "include"
    "${MACHINA_ROOT}/libs/libmc/include")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_LIST_DIR}/kernel.ld")

add_subdirectory(source/platform/bcm2837)

add_executable(kernel
    "source/platform/bcm2837/entrypoint.S"
    "source/uart.cc"
    "source/pmm.cc"
    "source/heap.cc"
    "source/mailbox.cc"
    "source/task.cc"
    "source/procfs.cc"
    "source/device.cc"
    "source/display.cc"
    "source/ramdsk.cc"
    "source/vfs.cc"
    "source/main.cc")
target_link_libraries(kernel bcm2837 libmc)
set_target_properties(kernel PROPERTIES
    OUTPUT_NAME "kernel8.elf"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(image
    ${CMAKE_OBJCOPY} ${CMAKE_BINARY_DIR}/kernel8.elf -O binary ${CMAKE_BINARY_DIR}/kernel8.img
    DEPENDS kernel "./kernel.ld")
