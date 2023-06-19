cmake_minimum_required(VERSION 3.17)

# set the project name
project(PadOS VERSION 0.1)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

include_directories(. Include ExternalLibs)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PADOS_C_CXX_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-multichar -Wno-sign-compare")
set(PADOS_C_CXX_FLAGS "${PADOS_C_CXX_FLAGS} -Wno-psabi") # Supress STL warnings

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_C_CXX_FLAGS} -Wsuggest-override -Wnoexcept")

# add the library
add_library(PadOS_Kernel)
add_library(PadOS_FATFS)
add_library(PadOS_System)
add_library(PadOS_Ptr)
add_library(PadOS_Signals)
add_library(PadOS_IO)
add_library(PadOS_GUI)
add_library(PadOS_ApplicationServer)
add_library(PadOS_WindowManager)

add_library(PadOS_Drivers_SDMMC EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_SPI EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_FT5x0x EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_GSLx680 EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_ADC EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_QSPI EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_SDRAM EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_USB EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_RealtimeClock EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_I2C EXCLUDE_FROM_ALL)

add_library(PadOS_Drivers_TLV493 EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_USART EXCLUDE_FROM_ALL)
add_library(PadOS_Drivers_WS2812B EXCLUDE_FROM_ALL)



target_sources(PadOS_Kernel PRIVATE
	SDCard/System/WindowManagerLayout.xml
	Init.cpp
	MemTest.cpp
	MemTest.h
	PadOS.natvis
	posix.cpp
)


add_subdirectory(Applications)
add_subdirectory(ApplicationServer)
add_subdirectory(Kernel)
add_subdirectory(SerialConsole)
add_subdirectory(System)
add_subdirectory(Include)
add_subdirectory(ExternalLibs)

