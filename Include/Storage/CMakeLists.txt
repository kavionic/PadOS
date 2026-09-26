# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_IO PRIVATE
	Directory.h
	DirectoryEntry.h
	DirIterator.h
	File.h
	FileReference.h
	FSNode.h
	MemFile.h
	NodeMonitor.h
	Path.h
	SeekableIO.h
	StandardPaths.h
	StreamableIO.h
	SymLink.h
	TempFile.h
)
