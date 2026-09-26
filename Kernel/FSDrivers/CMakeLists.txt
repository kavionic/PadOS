# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_KernelExt PRIVATE
	VirtualFSBase.cpp
)

add_subdirectory(BinFS)
add_subdirectory(FAT)
