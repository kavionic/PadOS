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
