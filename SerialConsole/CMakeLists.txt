# This file is part of PadOS.
#
# Copyright (c) 2022 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Drivers_Misc PRIVATE
	SerialDebugStreamDriver.cpp
)

target_sources(PadOS_KernelExt PRIVATE
	CommandHandlerFilesystem.cpp
	SerialCommandHandler.cpp
)
