# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_ApplicationServer PRIVATE
	FontGlyphCache.h
	FontManager.h
	ServerFontLibrary.h
	ServerFontFace.h
	ServerFontGlyph.h
	ServerFontGlyphTable.h
	ServerFontDrawCache.h
	ServerFontInstance.h
	ServerPreparedText.h
)
