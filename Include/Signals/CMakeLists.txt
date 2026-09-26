# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Signals PRIVATE
	RemoteSignal.h
	Signal.h
	SignalBase.h
	SignalSlotList.h
	SignalSystem.h
	SignalTarget.h
	SignalUnguarded.h
	Slot.h
	SlotBase.h
	VFConnector.h
	VFConnectorUnguarded.h
)
