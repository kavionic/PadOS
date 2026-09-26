# This file is part of PadOS.
#
# Copyright (c) 2022 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	CommandHandlerFilesystem.h
	FilesystemMessages.h
	SerialCommandHandler.h
	SerialDebugStreamDriver.h
	SerialProtocol.h
)
