cmake_minimum_required(VERSION 3.25)

# set the project name
project(PadOS VERSION 0.1)


include("CMake/Utils.cmake")

set(PADOS_BUILD_CONFIG "")
pados_find_build_config(PADOS_BUILD_CONFIG)

if(PADOS_BUILD_CONFIG)
	message(STATUS "Using PadOS config file: ${PADOS_BUILD_CONFIG}")
	include(${PADOS_BUILD_CONFIG})
else()
	message(STATUS "No external PadOS build config file found. Using defaults.")
endif()


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


include_directories(
	${CMAKE_SOURCE_DIR}/Include
	${CMAKE_SOURCE_DIR}/Applications
	${CMAKE_SOURCE_DIR}/ExternalLibs
	${CMAKE_SOURCE_DIR}/ExternalLibs/STM32/cmsis-core/Include
	${CMAKE_SOURCE_DIR}/ExternalLibs/STM32/cmsis-device-h7/Include
	${CMAKE_SOURCE_DIR}/ExternalLibs/pugixml/src
	${CMAKE_SOURCE_DIR}/ExternalLibs/json/include
	${CMAKE_SOURCE_DIR}/ExternalLibs/zlib
	${CMAKE_SOURCE_DIR}/ExternalLibs/libpng
	${CMAKE_SOURCE_DIR}/ExternalLibs/eigen
	${CMAKE_SOURCE_DIR}/ExternalLibs/googletest/googletest/include
	${CMAKE_SOURCE_DIR}/ExternalLibs/fmt/include
	${CMAKE_BINARY_DIR}/ExternalLibs/zlib
	${CMAKE_BINARY_DIR}/ExternalLibs/libpng
	.
)

set(PADOS_C_CXX_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-multichar -Wno-sign-compare")
set(PADOS_C_CXX_FLAGS "${PADOS_C_CXX_FLAGS} -Wno-psabi") # Supress STL warnings

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${PADOS_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PADOS_C_CXX_FLAGS} -Wsuggest-override -Wnoexcept -Wno-invalid-offsetof")


add_library(PadOS_Kernel)
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

set(PADOS_EXTERNAL_LIBS
	fmt
	pugixml
	png_static
	gtest
)


pados_add_compile_option(PADOS_OPT_DEBUG_LOOPER_LIST		DEBUG_LOOPER_LIST)
pados_add_compile_option(PADOS_OPT_DEBUG_HANDLE_OBJECT_LIST	DEBUG_HANDLE_OBJECT_LIST)
pados_add_compile_option(PADOS_OPT_USE_FMT_FORMATTING		PADOS_OPT_USE_FMT_FORMATTING)

if(PADOS_MODULE_UNITTESTS)
	add_library(PadOS_UnitTests)
endif()


pados_add_kernel_module(ON	PadOS_Kernel_Startup)
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

pados_add_kernel_library(ON	PadOS_Kernel)
pados_add_kernel_library(ON	PadOS_FATFS)
pados_add_kernel_library(ON	PadOS_Ptr)
pados_add_kernel_library(ON	PadOS_Signals)
pados_add_kernel_library(ON	PadOS_System)
pados_add_kernel_library(ON	PadOS_SystemIFlash)
pados_add_kernel_library(ON	PadOS_IO)
pados_add_kernel_library(ON	PadOS_Syscalls)
pados_add_kernel_library(ON	fmt)


set_libraries_section("iflash" ${PADOS_OPT_IFLASH_MODULES})


setup_unity_build_and_pch(5, "${CMAKE_CURRENT_SOURCE_DIR}/Include/Precompile.h")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/PadOSConfig.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/PadOSConfig.cmake
    @ONLY
)

install(TARGETS
	PadOS_Kernel
	PadOS_Syscalls
	PadOS_FATFS
	PadOS_System
	PadOS_SystemIFlash
	PadOS_Ptr
	PadOS_Signals
	PadOS_IO
	PadOS_GUI
	PadOS_ApplicationServer
	PadOS_WindowManager
	PadOS_DataTranslators

	PadOS_UnitTests
	${PADOS_KERNEL_MODULE_TARGETS}
	${PADOS_EXTERNAL_LIBS}
    EXPORT PadOSTargets
    ARCHIVE DESTINATION lib
)

install(DIRECTORY Include/ DESTINATION include
	FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
	PATTERN "CMakeLists.txt" EXCLUDE
	PATTERN "Precompile.h" EXCLUDE
)
install(FILES ${CMAKE_BINARY_DIR}/cmake/PadOSConfig.cmake DESTINATION lib/cmake/PadOS)
install(EXPORT PadOSTargets NAMESPACE PadOS:: DESTINATION lib/cmake/PadOS)

install(DIRECTORY ExternalLibs/STM32/cmsis-core/Include/ DESTINATION include/cmsis-core)
install(DIRECTORY ExternalLibs/STM32/cmsis-device-h7/Include/ DESTINATION include/cmsis-device-h7)

install(TARGETS zlib
    ARCHIVE DESTINATION lib
)

install(FILES
    ${CMAKE_SOURCE_DIR}/ExternalLibs/zlib/zlib.h
    ${CMAKE_BINARY_DIR}/ExternalLibs/zlib/zconf.h
    DESTINATION include
)

