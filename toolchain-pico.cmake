set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMALE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TRIPLET arm-none-eabi)

execute_process(
    COMMAND which ${TRIPLET}-gcc
    OUTPUT_VARIABLE BINUTILS_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)

#set(CMAKE_FIND_ROOT_PATH ARM_TOOLCHAIN_DIR)

set(CMAKE_C_COMPILER ${TRIPLET}-gcc)
set(CMAKE_ASM_COMPILER ${TRIPLET}-gcc)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(MCPU_FLAGS "-mthumb -mcpu=cortex-m0plus")

set(CMAKE_COMMON_FLAGS "-nostdlib -Wall ")

set(CMAKE_C_FLAGS "${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG "-g ${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_DEBUG "-g ${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE "-O2 ${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_RELEASE "${MCPU_FLAGS} ${CMAKE_COMMON_FLAGS}")

set(CMAKE_SYSROOT ${ARM_TOOLCHAIN_DIR})

