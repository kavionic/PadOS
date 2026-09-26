# This file is part of PadOS.
#
# Copyright (c) 2022 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	USBClassDriverDevice.h
	USBClassDriverHost.h
	USBClientControl.h
	USBCommon.h
	USBDevice.h
	USBDriver.h
	USBEndpointState.h
	USBHost.h
	USBHostControl.h
	USBHostEnumerator.h
	USBHostHub.h
	DevFS/USBDeviceRegistry.h
	USBProtocol.h
	USBProtocolCDC.h
	USBProtocolHID.h
	USBProtocolHub.h
	USBProtocolMSC.h
)

add_subdirectory(ClassDrivers)
