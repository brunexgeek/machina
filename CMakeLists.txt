cmake_minimum_required(VERSION 2.8)

set(CMAKE_USE_RELATIVE_PATHS false)
set(MACHINA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(MACHINA_TARGET "Raspberry Pi Model B" CACHE STRING "Target Raspberry board")
set_property(CACHE  MACHINA_TARGET PROPERTY STRINGS
#    "RPI-1A"
#    "RPI-1B"
#    "RPI-1A+"
#    "RPI-1B+"
    "RPI-2B"
    "RPI-3B" )

set(TOOLCHAIN_HOME "/opt/arm-rpi-4.9.3-linux-gnueabihf" CACHE STRING "")
set(TOOLCHAIN_PREFIX "bin/aarch64-none-linux-gnu-" CACHE STRING "")

enable_language(ASM)
if(CMAKE_ASM_COMPILER_WORKS)
	set(can_use_assembler TRUE)
endif(CMAKE_ASM_COMPILER_WORKS)

set(CMAKE_C_COMPILER   "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_AR           "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}ar")
set(CMAKE_LD           "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}ld")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_RANLIB       "${TOOLCHAIN_HOME}/${TOOLCHAIN_PREFIX}ranlib")

if (MACHINA_TARGET STREQUAL "")
    set(MACHINA_TARGET "RPI-3B")
endif()

if (MACHINA_TARGET STREQUAL "RPI-3B")
	set(TARGET_ARCH "-march=armv8-a -mtune=cortex-a53")
    set(RPIGEN 3)
elseif (MACHINA_TARGET STREQUAL "RPI-2B")
    set(TARGET_ARCH "-march=armv7ve -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard")
    set(RPIGEN 2)
else()
    set(TARGET_ARCH "-march=armv6j -mtune=arm1176jzf-s -mfloat-abi=hard")
    set(RPIGEN 1)
endif()

set(COMMOM_FLAGS "-Wfatal-errors -O0 -g -ffreestanding -Wl,--no-undefined -Wall -Wextra -Wconversion -Werror=return-type  -Wno-psabi -fsigned-char -fno-builtin -nostdinc -nostdlib -pedantic -Wuninitialized -Winit-self -D__arm__=1")

set(CMAKE_C_FLAGS   "${TARGET_ARCH} ${COMMOM_FLAGS} -DRPIGEN=${RPIGEN} -std=c11")
set(CMAKE_ASM_FLAGS "${TARGET_ARCH} ${COMMOM_FLAGS} -DRPIGEN=${RPIGEN}")
set(CMAKE_CXX_FLAGS "${TARGET_ARCH} ${COMMOM_FLAGS} -DRPIGEN=${RPIGEN} -fno-exceptions -fno-rtti -std=c++11")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_subdirectory(libs/libmc)
add_subdirectory(kernel)


if (CMAKE_CXX_COMPILER_VERSION)
    set(MACHINA_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} (ver ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_COMPILER_IS_CLANGCXX)
    set(MACHINA_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} (ver ${CMAKE_CLANG_REGEX_VERSION})")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(MACHINA_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} (ver ${CMAKE_GCC_REGEX_VERSION})")
else()
    set(MACHINA_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}")
endif()
string(STRIP "${MACHINA_COMPILER_STR}" MACHINA_COMPILER_STR)
message("        C++ Compiler:  ${MACHINA_COMPILER_STR}")
message(" C++ flags (Release):  ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
message("   C++ flags (Debug):   ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
message("          C Compiler:  ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1}")
message("   C flags (Release):  ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
message("     C flags (Debug):  ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
if(WIN32)
    message("  LD flags (Release): ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    message("    LD flags (Debug):   ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
else()
    message("  LD flags (Release): ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
    message("    LD flags (Debug):   ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
endif()
message("")