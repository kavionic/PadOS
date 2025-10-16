set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Find GCC.

#unset(PADOS_C_COMPILER CACHE)
#unset(PADOS_CXX_COMPILER CACHE)

find_program(PADOS_C_COMPILER arm-unknown-pados-eabi-gcc   PATHS $ENV{PADOS_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)
find_program(PADOS_CXX_COMPILER arm-unknown-pados-eabi-g++ PATHS $ENV{PADOS_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)

message("C Compiler: ${PADOS_C_COMPILER}.")
message("C++ Compiler: ${PADOS_CXX_COMPILER}.")

if("${PADOS_C_COMPILER}" STREQUAL "PADOS_C_COMPILER-NOTFOUND")
        message(WARNING "C Compiler not found, please specify a search path with \"PADOS_TOOLCHAIN_PATH\".")
endif()
if("${PADOS_CXX_COMPILER}" STREQUAL "PADOS_CXX_COMPILER-NOTFOUND")
        message(WARNING "C++ Compiler not found, please specify a search path with \"PADOS_TOOLCHAIN_PATH\".")
endif()

if("$ENV{PADOS_STM32_TOOLKIT}-NOTFOUND" STREQUAL "-NOTFOUND")
        message(FATAL_ERROR "PADOS_STM32_TOOLKIT not set.")
endif()

#unset(CMAKE_C_COMPILER CACHE)
#unset(CMAKE_CXX_COMPILER CACHE)

set(CMAKE_C_COMPILER   ${PADOS_C_COMPILER}   CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${PADOS_CXX_COMPILER} CACHE FILEPATH "C++ compiler")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(STM32H7 STM32H753xx DEBUG ARM_MATH_CM7=true)

include_directories(
	PadOS/ExternalLibs/STM32/cmsis-core/Include
	PadOS/ExternalLibs/STM32/cmsis-device-h7/Include
)

include_directories(
	${CMAKE_SOURCE_DIR}/PadOS/Include
	${CMAKE_SOURCE_DIR}/PadOS/Applications
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs/json/include
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs/zlib
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs/libpng
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs/eigen
	${CMAKE_SOURCE_DIR}/PadOS/ExternalLibs/googletest/googletest/include
	${CMAKE_BINARY_DIR}/PadOS/ExternalLibs/zlib
	${CMAKE_BINARY_DIR}/PadOS/ExternalLibs/libpng
	.
)

set(PADOS_SYSTEM_C_CXX_FLAGS "-mcpu=cortex-m7 -ffunction-sections -fdata-sections -fstack-usage -mthumb -mno-unaligned-access -ffast-math -mfpu=fpv5-d16 -mfloat-abi=hard -fexceptions -funwind-tables")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_SYSTEM_C_CXX_FLAGS} -D_DEFAULT_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_SYSTEM_C_CXX_FLAGS} -D_DEFAULT_SOURCE --std=gnu++23")

set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m7 -Wl,--gc-sections -static -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb")
