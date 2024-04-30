
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR riscv)
 
SET(CMAKE_C_COMPILER_WORKS TRUE)
SET(CMAKE_CXX_COMPILER_WORKS TRUE)

if (JUDGE_ONLINE STREQUAL "true")
    SET(TOOLCHAIN_PREFIX "riscv64-unknown-elf") # !! DO NOT CHANGE THIS LINE !!
else()
    SET(TOOLCHAIN_PREFIX "riscv64-unknown-elf") # NOTE: change this line if you use other toolchains
endif()

SET(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}-gcc")
SET(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}-g++")
SET(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}-as")
SET(CMAKE_AR "${TOOLCHAIN_PREFIX}-ar")
SET(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}-objcopy")
SET(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}-objdump")

set(CMAKE_CXX_LINK_FLAGS "${LINK_FLAGS} -march=rv32im -mabi=ilp32 -nostdlib -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -mno-relax")

SET(RISCV_COMPILE_OPTIONS "-march=rv32im" "-mabi=ilp32" "-O2" "-nostdlib" "-fno-PIC" "-mcmodel=medany" "-fno-builtin")

file(GLOB_RECURSE CPP_SRC_LIST *.cpp)

foreach(src ${CPP_SRC_LIST})
    string(REGEX MATCH "test/.*" relative_path ${src})
    string(REGEX REPLACE "test/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    message(STATUS "Found test program " ${target_name})

    add_executable(${target_name} ${src})
    set_target_properties(${target_name} PROPERTIES COMPILE_OPTIONS "${RISCV_COMPILE_OPTIONS}")
    target_link_options(${target_name} PRIVATE -Ttext 0x80000000)

    add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} ARGS -d -s $<TARGET_FILE:${target_name}> > ${target_name}.asm
    )

endforeach()