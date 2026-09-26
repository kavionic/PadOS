# This file is part of PadOS.
#
# Copyright (c) 2025 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
	Exit.cpp
)

if(PADOS_MODULE_POSIX_SPAWN)
	target_sources(PadOS_System PRIVATE
		Process.cpp
	)
endif()
