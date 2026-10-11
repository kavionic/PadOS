# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	RA8875Driver.cpp
	KDisplayDriver.cpp
	KDisplayDriverLineRender.cpp
	KDisplayDriverProtocol.cpp
)
