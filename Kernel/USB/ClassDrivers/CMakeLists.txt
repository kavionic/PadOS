# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Drivers_USB PRIVATE
	USBCDCBuffers.cpp
	USBClientCDCChannel.cpp
	USBClientClassCDC.cpp
)

if(PADOS_MODULE_USB_HOST)
	target_sources(PadOS_Drivers_USB PRIVATE
	USBHIDBootKeyboardDriver.cpp
	USBHIDMouseDriver.cpp
	USBHIDDriver.cpp
	USBHIDReportParser.cpp
	USBHostCDCChannel.cpp
	USBHostClassCDC.cpp
	USBHostHIDInterface.cpp
	USBHostClassHID.cpp
	USBHostClassMSC.cpp
	)
endif()
