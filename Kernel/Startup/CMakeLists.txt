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
