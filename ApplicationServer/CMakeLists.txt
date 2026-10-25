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

add_subdirectory(Fonts)

# Group the application server and font implementations to enable cross-file
# optimization, including inlining between font and glyph-cache operations.
set_target_properties(PadOS_ApplicationServer PROPERTIES UNITY_BUILD_MODE GROUP)
get_target_property(applicationServerSources PadOS_ApplicationServer SOURCES)
set_source_files_properties(${applicationServerSources}
    TARGET_DIRECTORY PadOS_ApplicationServer
    PROPERTIES UNITY_GROUP appserver
)
set_source_files_properties(ServerView.cpp
    TARGET_DIRECTORY PadOS_ApplicationServer
    PROPERTIES UNITY_GROUP views
)

target_link_libraries(PadOS_ApplicationServer PUBLIC xkbcommon::xkbcommon freetype)
