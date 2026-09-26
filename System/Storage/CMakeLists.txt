# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_IO PRIVATE
	Directory.cpp
	File.cpp
	FileReference.cpp
	FSNode.cpp
	MemFile.cpp
	NodeMonitor.cpp
	Path.cpp
	SeekableIO.cpp
	StandardPaths.cpp
	Storage.ddoc
	StreamableIO.cpp
	Symlink.cpp
	TempFile.cpp
)
