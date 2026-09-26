# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Signals PRIVATE
	SignalBase.cpp
	SignalSystem.cpp
	SignalTarget.cpp
	SlotBase.cpp
)
