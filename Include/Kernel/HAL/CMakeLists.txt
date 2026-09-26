# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE 
	DigitalPort.h
	DigitalPort_ATSAM.h
	DigitalPort_STM32.h
	DMA.h
	Peripherals.h
	SAME70System.h
)

add_subdirectory(ATSAM)
add_subdirectory(STM32)
