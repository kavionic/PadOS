# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	USBClientCDCChannel.h
	USBClientClassCDC.h
	USBHIDBootKeyboardDriver.h
	USBHIDMouseDriver.h
	USBHIDDriver.h
	USBHIDReportParser.h
	USBHostCDCChannel.h
	USBHostClassCDC.h
	USBHostClassHID.h
	USBHostClassMSC.h
	USBHostHIDInterface.h
)
