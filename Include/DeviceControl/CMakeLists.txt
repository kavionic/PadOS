# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	BME280.h
	HID.h
	InputDevice.h
	Display.h
	DisplayFont.h
	I2C.h
	INA3221.h
	SDCARD.h
	SPI.h
	TLV493D.h
	USART.h
	USB.h
	WS2812B.h
)
