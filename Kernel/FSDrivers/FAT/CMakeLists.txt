# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_FATFS PRIVATE
	FATCodePage.cpp
	FATCodePage.h
	FATClusterSectorIterator.cpp
	FATClusterSectorIterator.h
	FATDirectoryIterator.cpp
	FATDirectoryIterator.h
	FATDirectoryNode.cpp
	FATDirectoryNode.h
	FATFileNode.cpp
	FATFileNode.h
	FATFilesystem.cpp
	FATInode.cpp
	FATInode.h
	FATTable.cpp
	FATTable.h
	FATTableIterator.cpp
	FATTableIterator.h
	FATVolume.cpp
	FATVolume.h
)
