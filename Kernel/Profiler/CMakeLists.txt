# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

if(PADOS_MODULE_GPROF_SAMPLING)
	target_sources(PadOS_Kernel PRIVATE
		KGProfData.h
		KGProfSampler.cpp
	)

	target_sources(PadOS_KernelExt PRIVATE
		KGProf.cpp
	)

	if(NOT PADOS_GPROF_SAMPLING_TIMER STREQUAL "SysTick")
		target_sources(PadOS_Kernel PRIVATE KGProfTimer_STM32.cpp)
	endif()
endif()

if(PADOS_MODULE_GPROF_CALL_GRAPH)
	set_source_files_properties(
		KGProf.cpp KGProfSampler.cpp KGProfTimer_STM32.cpp
		TARGET_DIRECTORY PadOS_Kernel PadOS_KernelExt PROPERTIES
		COMPILE_OPTIONS "-fno-profile"
		SKIP_UNITY_BUILD_INCLUSION ON
		SKIP_PRECOMPILE_HEADERS ON
	)
endif()
