SET(CMAKE_SYSTEM_NAME Generic)

set(TOOLCHAIN_HOME "/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-elf/" CACHE STRING "")
set(TOOLCHAIN_PREFIX "aarch64-none-elf-" CACHE STRING "")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_AR           "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}ar")
set(CMAKE_LD           "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}ld")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_RANLIB       "${TOOLCHAIN_HOME}/bin/${TOOLCHAIN_PREFIX}ranlib")
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

message("C++ Compiler: ${CMAKE_CXX_COMPILER}")

set(TARGET_ARCH "-march=armv8-a -mcpu=cortex-a53+fp+simd")
set(RPIGEN 3)

set(COMMOM_FLAGS "-Wfatal-errors -O0 -g -ffreestanding -Wl,--no-undefined -Wall -Wextra -Wconversion -Werror=return-type  -Wno-psabi -fsigned-char -fno-builtin -nostdinc -nostdlib -pedantic -Wuninitialized -Winit-self -D__arm__=1")

set(CMAKE_C_FLAGS   "${TARGET_ARCH} ${COMMOM_FLAGS} -DRPIGEN=${RPIGEN} -std=c11")
set(CMAKE_ASM_FLAGS "${TARGET_ARCH} ${COMMOM_FLAGS} -fno-threadsafe-statics -DRPIGEN=${RPIGEN}")
set(CMAKE_CXX_FLAGS "${TARGET_ARCH} ${COMMOM_FLAGS} -DRPIGEN=${RPIGEN} -fno-exceptions -fno-rtti -std=c++11")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")