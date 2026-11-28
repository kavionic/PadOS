# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE
	KDisplayBrushSampler.h
	KDisplayImageResource.h
	KDisplayBrush.h
	KDisplayResources.h
	RA8875Driver.h
	RA8875Registers.h
	KDisplayDriver.h
	KDisplayText.h
	BlitterUtils.h
)
