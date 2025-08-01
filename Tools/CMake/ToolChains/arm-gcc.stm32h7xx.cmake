set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Find GCC.
find_program(PADOS_C_COMPILER arm-none-eabi-gcc   PATHS $ENV{PADOS_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)
find_program(PADOS_CXX_COMPILER arm-none-eabi-g++ PATHS $ENV{PADOS_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)

if("${PADOS_C_COMPILER}" STREQUAL "${PADOS_C_COMPILER}-NOTFOUND")
        message(FATAL_ERROR "C Compiler not found, please specify a search path with \"PADOS_TOOLCHAIN_PATH\".")
endif()
if("${PADOS_CXX_COMPILER}" STREQUAL "${PADOS_C_COMPILER}-NOTFOUND")
        message(FATAL_ERROR "C++ Compiler not found, please specify a search path with \"PADOS_TOOLCHAIN_PATH\".")
endif()

if("$ENV{PADOS_STM32_TOOLKIT}-NOTFOUND" STREQUAL "-NOTFOUND")
        message(FATAL_ERROR "PADOS_STM32_TOOLKIT not set.")
endif()

set(CMAKE_C_COMPILER   ${PADOS_C_COMPILER}   CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${PADOS_CXX_COMPILER} CACHE FILEPATH "C++ compiler")

# Disable compiler checks.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

add_compile_definitions(STM32H7 STM32H753xx DEBUG ARM_MATH_CM7=true __machine_ino_t_defined __ino_t=int64_t)

include_directories(
	"$ENV{PADOS_STM32_TOOLKIT}/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
	"$ENV{PADOS_STM32_TOOLKIT}/Drivers/CMSIS/Core/Include"
)

set(PADOS_SYSTEM_C_CXX_FLAGS "-mcpu=cortex-m7 -ffunction-sections -fdata-sections -fstack-usage -mthumb -ffast-math -mfpu=fpv5-d16 -mfloat-abi=hard -mtp=cp15 -mtls-dialect=gnu2 -fexceptions -funwind-tables")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_SYSTEM_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_SYSTEM_C_CXX_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m7 -Wl,--gc-sections -static -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb")

