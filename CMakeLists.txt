cmake_minimum_required(VERSION 3.17)

# set the project name
project(PadOS VERSION 0.1)

option(PADOS_OPT_DEBUG_LOOPER_LIST		"Keep a global list of PLooper objects for debugging."			OFF)
option(PADOS_OPT_DEBUG_HANDLE_OBJECT_LIST	"Keep a global list of KNamedObject objects for debugging."		OFF)
option(PADOS_OPT_USE_FMT_FORMATTING		"Use fmt::format instead of std::format for smaller memory usage."	OFF)

option(PADOS_MODULE_UNITTESTS			"Build the unit tests" OFF)
option(PADOS_DRIVER_MultiMotor			"Build driver" OFF)
option(PADOS_DRIVER_SDMMC			"Build driver" OFF)
option(PADOS_DRIVER_SPI				"Build driver" OFF)
option(PADOS_DRIVER_FT5x0x			"Build driver" OFF)
option(PADOS_DRIVER_GSLx680			"Build driver" OFF)
option(PADOS_DRIVER_ADC				"Build driver" OFF)
option(PADOS_DRIVER_QSPI			"Build driver" OFF)
option(PADOS_DRIVER_SDRAM			"Build driver" OFF)
option(PADOS_DRIVER_USB				"Build driver" ON)
option(PADOS_DRIVER_RealtimeClock		"Build driver" ON)
option(PADOS_DRIVER_I2C				"Build driver" OFF)
option(PADOS_DRIVER_TLV493			"Build driver" OFF)
option(PADOS_DRIVER_USART			"Build driver" OFF)
option(PADOS_DRIVER_WS2812B			"Build driver" OFF)


set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

include_directories(. Include ExternalLibs)

set(PADOS_C_CXX_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-multichar -Wno-sign-compare")
set(PADOS_C_CXX_FLAGS "${PADOS_C_CXX_FLAGS} -Wno-psabi") # Supress STL warnings

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_C_CXX_FLAGS} -Wsuggest-override -Wnoexcept -Wno-invalid-offsetof")


add_compile_definitions(FMT_OPTIMIZE_SIZE=2)

#link_libraries(PRIVATE runtimes::ready)

add_executable(PadOS_KernelImage)

add_library(PadOS_Kernel)
add_library(PadOS_Startup)
add_library(PadOS_Syscalls)
add_library(PadOS_FATFS)
add_library(PadOS_System)
add_library(PadOS_SystemIFlash)
add_library(PadOS_Ptr)
add_library(PadOS_Signals)
add_library(PadOS_IO)
add_library(PadOS_GUI)
add_library(PadOS_ApplicationServer)
add_library(PadOS_WindowManager)
add_library(PadOS_DataTranslators)

target_compile_definitions(PadOS_KernelImage PUBLIC __pados_kernel__)

set(PADOS_KERNEL_MODULES "")

macro(pados_add_compile_option option define)
    if(${opt})
	add_compile_definitions(${define}=1)
    endif()
endmacro()

macro(pados_add_kernel_module opt lib)
    if(${opt})
        list(APPEND PADOS_KERNEL_MODULES ${lib})
	add_library(${lib} EXCLUDE_FROM_ALL)
    endif()
endmacro()


pados_add_compile_option(PADOS_OPT_DEBUG_LOOPER_LIST		DEBUG_LOOPER_LIST)
pados_add_compile_option(PADOS_OPT_DEBUG_HANDLE_OBJECT_LIST	DEBUG_HANDLE_OBJECT_LIST)
pados_add_compile_option(PADOS_OPT_USE_FMT_FORMATTING		PADOS_OPT_USE_FMT_FORMATTING)

if(PADOS_MODULE_UNITTESTS)
	add_library(PadOS_UnitTests)
endif()

#add_library(PadOS_Drivers_Misc)
#list(APPEND PADOS_KERNEL_MODULES PadOS_Drivers_Misc)

pados_add_kernel_module(ON	PadOS_Drivers_Misc)

pados_add_kernel_module(PADOS_DRIVER_MultiMotor		PadOS_Drivers_MultiMotor)
pados_add_kernel_module(PADOS_DRIVER_SDMMC		PadOS_Drivers_SDMMC)
pados_add_kernel_module(PADOS_DRIVER_SPI		PadOS_Drivers_SPI)
pados_add_kernel_module(PADOS_DRIVER_FT5x0x		PadOS_Drivers_FT5x0x)
pados_add_kernel_module(PADOS_DRIVER_GSLx680		PadOS_Drivers_GSLx680)
pados_add_kernel_module(PADOS_DRIVER_ADC		PadOS_Drivers_ADC)
pados_add_kernel_module(PADOS_DRIVER_QSPI		PadOS_Drivers_QSPI)
pados_add_kernel_module(PADOS_DRIVER_SDRAM		PadOS_Drivers_SDRAM)
pados_add_kernel_module(PADOS_DRIVER_USB		PadOS_Drivers_USB)
pados_add_kernel_module(PADOS_DRIVER_RealtimeClock	PadOS_Drivers_RealtimeClock)
pados_add_kernel_module(PADOS_DRIVER_I2C		PadOS_Drivers_I2C)
pados_add_kernel_module(PADOS_DRIVER_TLV493		PadOS_Drivers_TLV493)
pados_add_kernel_module(PADOS_DRIVER_USART		PadOS_Drivers_USART)
pados_add_kernel_module(PADOS_DRIVER_WS2812B		PadOS_Drivers_WS2812B)

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

add_dependencies(png_static zlib)
add_dependencies(PadOS_DataTranslators png_static)




target_link_libraries(PadOS_KernelImage
	-Wl,--start-group
	PadOS_SystemInit
	PadOS_Kernel
	PadOS_FATFS
	PadOS_Ptr
	PadOS_Signals
	PadOS_System
	PadOS_SystemIFlash
	PadOS_IO
	-Wl,--whole-archive
	PadOS_Startup
	PadOS_Syscalls
	${PADOS_KERNEL_MODULES}
	-Wl,--no-whole-archive
	c
	pthread
	fmt
	-Wl,--end-group
)

target_link_options(PadOS_KernelImage PRIVATE
	"-L${CMAKE_SOURCE_DIR}/LinkerScripts"
	"-TPadOS_KernelImage.ld"
	"-Wl,-Map=${CMAKE_BINARY_DIR}/PadOS_KernelImage.map"
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/PadOS/PadOS_KernelImage.hex
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            ${CMAKE_BINARY_DIR}/PadOS/PadOS_KernelImage.elf
            ${CMAKE_BINARY_DIR}/PadOS/PadOS_KernelImage.hex
    DEPENDS PadOS_KernelImage
    VERBATIM
)

add_custom_target(PadOS_KernelImage_hex ALL DEPENDS ${CMAKE_BINARY_DIR}/PadOS/PadOS_KernelImage.hex)


get_property(all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)


# Minimum number of C++ source files required before we enable PCH
set(MIN_CXX_SOURCES 5)   # tweak this threshold to taste

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

	if(PADOS_OPT_USE_UNITY_BUILD)
		message(STATUS "Enabling unity build for target ${buildTarget}")
		set_target_properties(${buildTarget} PROPERTIES UNITY_BUILD ON)
	endif()

	if(PADOS_OPT_USE_PCH)
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
			target_precompile_headers(${buildTarget} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${PADOS_PCH_HEADER}>")
		else()
			message(STATUS "Skipping PCH for target ${buildTarget} (${cxx_count} C++ sources)")
		endif()
	endif()
endforeach()

