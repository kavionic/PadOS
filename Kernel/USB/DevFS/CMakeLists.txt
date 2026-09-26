# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Drivers_USB PRIVATE
	USBDeviceControlInode.cpp
	USBDeviceControlInode.h
	USBDeviceInterfaceInode.cpp
	USBDeviceInterfaceInode.h
	USBDeviceRegistry.cpp
)
