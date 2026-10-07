# This file is part of PadOS.
#
# Copyright (c) 2025 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel_Startup PRIVATE
	KStartup.cpp
)

target_sources(PadOS_Kernel_Unconditional PRIVATE
	KVectorTable.cpp
)

target_sources(PadOS_Kernel PRIVATE KMemoryStartup.cpp)
set_source_files_properties(KMemoryStartup.cpp TARGET_DIRECTORY PadOS_Kernel PROPERTIES
    COMPILE_OPTIONS "-fno-profile;-mgeneral-regs-only"
    SKIP_UNITY_BUILD_INCLUSION ON
    SKIP_PRECOMPILE_HEADERS ON
)
