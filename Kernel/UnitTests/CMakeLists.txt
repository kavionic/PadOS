# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel_Unconditional PRIVATE
	USBHIDReportParser_unittest.cpp
)

if(PADOS_DRIVER_USB)
    target_sources(PadOS_Kernel_Unconditional PRIVATE
        USBClientCDCRecovery_unittest.cpp
        USBReceiveCapacity_unittest.cpp
    )
endif()
