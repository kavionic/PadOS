# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	DigitalPort.cpp
#	SAME70System.cpp
)

add_subdirectory(ATSAM)
add_subdirectory(STM32)

target_sources(PadOS_Kernel PRIVATE MPU_ARMv7M.cpp)
set_source_files_properties(MPU_ARMv7M.cpp TARGET_DIRECTORY PadOS_Kernel PROPERTIES
    COMPILE_OPTIONS "-fno-profile;-mgeneral-regs-only"
    SKIP_UNITY_BUILD_INCLUSION ON
    SKIP_PRECOMPILE_HEADERS ON
)
