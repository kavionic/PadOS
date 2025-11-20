cmake_minimum_required(VERSION 3.17)

# set the project name
project(PadOS VERSION 0.1)

option(PADOS_MODULE_UNITTESTS "Build the unit tests" OFF)
option(PADOS_DRIVER_MultiMotor	"Build driver" OFF)
option(PADOS_DRIVER_SDMMC	"Build driver" OFF)
option(PADOS_DRIVER_SPI		"Build driver" OFF)
option(PADOS_DRIVER_FT5x0x	"Build driver" OFF)
option(PADOS_DRIVER_GSLx680	"Build driver" OFF)
option(PADOS_DRIVER_ADC		"Build driver" OFF)
option(PADOS_DRIVER_QSPI	"Build driver" OFF)
option(PADOS_DRIVER_SDRAM	"Build driver" OFF)
option(PADOS_DRIVER_USB		"Build driver" ON)
option(PADOS_DRIVER_RealtimeClock "Build driver" ON)
option(PADOS_DRIVER_I2C		"Build driver" OFF)
option(PADOS_DRIVER_TLV493	"Build driver" OFF)
option(PADOS_DRIVER_USART	"Build driver" OFF)
option(PADOS_DRIVER_WS2812B	"Build driver" OFF)


set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

include_directories(. Include ExternalLibs)

set(PADOS_C_CXX_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-multichar -Wno-sign-compare")
set(PADOS_C_CXX_FLAGS "${PADOS_C_CXX_FLAGS} -Wno-psabi") # Supress STL warnings

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_C_CXX_FLAGS} -Wsuggest-override -Wnoexcept -Wno-invalid-offsetof")

#link_libraries(PRIVATE runtimes::ready)

# add the library
add_library(PadOS_Kernel)
add_library(PadOS_Syscalls)
add_library(PadOS_FATFS)
add_library(PadOS_System)
add_library(PadOS_Ptr)
add_library(PadOS_Signals)
add_library(PadOS_IO)
add_library(PadOS_GUI)
add_library(PadOS_ApplicationServer)
add_library(PadOS_WindowManager)
add_library(PadOS_DataTranslators)

if(PADOS_MODULE_UNITTESTS)
	add_library(PadOS_UnitTests)
endif()
if(PADOS_DRIVER_MultiMotor)
	add_library(PadOS_Drivers_MultiMotor EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_SDMMC)
	add_library(PadOS_Drivers_SDMMC EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_SPI)
	add_library(PadOS_Drivers_SPI EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_FT5x0x)
	add_library(PadOS_Drivers_FT5x0x EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_GSLx680)
	add_library(PadOS_Drivers_GSLx680 EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_ADC)
	add_library(PadOS_Drivers_ADC EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_QSPI)
	add_library(PadOS_Drivers_QSPI EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_SDRAM)
	add_library(PadOS_Drivers_SDRAM EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_USB)
	add_library(PadOS_Drivers_USB EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_RealtimeClock)
	add_library(PadOS_Drivers_RealtimeClock EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_I2C)
	add_library(PadOS_Drivers_I2C EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_TLV493)
	add_library(PadOS_Drivers_TLV493 EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_USART)
	add_library(PadOS_Drivers_USART EXCLUDE_FROM_ALL)
endif()
if(PADOS_DRIVER_WS2812B)
	add_library(PadOS_Drivers_WS2812B EXCLUDE_FROM_ALL)
endif()

target_sources(PadOS_Kernel PRIVATE
	SDCard/System/WindowManagerLayout.xml
	MemTest.cpp
	MemTest.h
	PadOS.natvis
)


add_subdirectory(ExternalLibs)
add_subdirectory(Applications)
add_subdirectory(ApplicationServer)
add_subdirectory(DataTranslation)
add_subdirectory(DataTranslators)
add_subdirectory(Kernel)
add_subdirectory(SerialConsole)
add_subdirectory(System)
if(PADOS_MODULE_UNITTESTS)
	add_subdirectory(UnitTests)
endif()
add_subdirectory(Include)

get_property(all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)


# Minimum number of C++ source files required before we enable PCH
set(MIN_CXX_SOURCES 10)   # tweak this threshold to taste

set(PADOS_PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Include/Precompile.h")

foreach(buildTarget ${all_targets})
	# Skip imported targets (toolchain, external stuff)
	get_target_property(is_imported ${buildTarget} IMPORTED)
	if(is_imported)
		continue()
	endif()

	# Skip pure INTERFACE libraries
	get_target_property(type ${buildTarget} TYPE)
	if(type STREQUAL "INTERFACE_LIBRARY")
		continue()
	endif()

	# Get the sources for this target
	get_target_property(targetSources ${buildTarget} SOURCES)
	if(NOT targetSources)
		continue()
	endif()

	message(STATUS "Enabling unity build for target ${buildTarget}")
	set_target_properties(${buildTarget} PROPERTIES UNITY_BUILD ON)

	# Count C++ sources
	set(cxx_count 0)
	foreach(src ${targetSources})
		# Ignore generator expressions etc.
		if(src MATCHES "^\\$<")
			continue()
		endif()

		if(src MATCHES "\\.(cxx|cpp|cc|C)$")
			math(EXPR cxx_count "${cxx_count} + 1")
		endif()
	endforeach()

	# Only enable PCH if we have enough C++ files
	if(cxx_count GREATER_EQUAL MIN_CXX_SOURCES)
		message(STATUS "Enabling PCH for target ${buildTarget} (${cxx_count} C++ sources)")
		target_precompile_headers(${buildTarget} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${PADOS_PCH_HEADER}>"
	)
	else()
		message(STATUS "Skipping PCH for target ${buildTarget} (${cxx_count} C++ sources)")
	endif()
endforeach()

