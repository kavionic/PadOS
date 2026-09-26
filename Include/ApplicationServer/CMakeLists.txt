# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_ApplicationServer PRIVATE
	ApplicationServer.h
	DisplayDriver.h
	Font.h
	Protocol.h
	ServerBitmap.h
)

add_subdirectory(Drivers)
add_subdirectory(Fonts)
