file(GLOB SOURCE
    "source/*.cc"
    "source/arm/*.S")

include_directories(
    "include")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_LIST_DIR}/kernel.ld")

add_executable(kernel ${SOURCE})
set_target_properties(kernel PROPERTIES
    OUTPUT_NAME "kernel.elf"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(kernel.img
    ${CMAKE_OBJCOPY} ${CMAKE_BINARY_DIR}/kernel.elf -O binary ${CMAKE_BINARY_DIR}/kernel.img)