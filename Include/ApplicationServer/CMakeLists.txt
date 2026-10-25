# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_ApplicationServer PRIVATE
	ApplicationServer.h
	DisplayDriver.h
	DisplayDriverInvoker.h
	FontProtocol.h
	Protocol.h
	KeyboardManager.h
	KeyboardProtocol.h
	ServerBitmap.h
)

add_subdirectory(Fonts)
