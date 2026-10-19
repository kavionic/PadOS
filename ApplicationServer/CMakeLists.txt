# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_ApplicationServer PRIVATE
	ApplicationServer.cpp
	DisplayDriver.cpp
	KeyboardManager.cpp
	ServerApplication.cpp
	ServerApplication.h
	ServerBitmap.cpp
	ServerView.cpp
	ServerView.h
)

target_link_libraries(PadOS_ApplicationServer PUBLIC xkbcommon::xkbcommon)
