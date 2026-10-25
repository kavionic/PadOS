# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_ApplicationServer PRIVATE
	FontGlyphCache.cpp
	FontManager.cpp
	ServerFontLibrary.cpp
	ServerFontFace.cpp
	ServerFontGlyph.cpp
	ServerFontGlyphTable.cpp
	ServerFontDrawCache.cpp
	ServerFontInstance.cpp
)
