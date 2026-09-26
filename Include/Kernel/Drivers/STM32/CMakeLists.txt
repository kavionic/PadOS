# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	I2CDriver.h
	SDMMCDriver_STM32.h
	SPIDriver.h
	TLV493DDriver.h
	USARTDriver.h
	WS2812BDriver.h
)
