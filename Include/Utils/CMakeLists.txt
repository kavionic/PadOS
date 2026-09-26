# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
	ANSIEscapeCodeParser.h
	Base64Codec.h
	Beep.h
	CircularBuffer.h
	combsort.h
	EasingCurve.h
	EnumBitmask.h
	EnumUtils.h
	FactoryAutoRegistrator.h
	HashCalculator.h
	InertialScroller.h
	IntrusiveList.h
	JSON.h
	Logging.h
	MessagePort.h
	POSIXTokenizer.h
	RelocMemSection.h
	String.h
	TerminalLineEditor.h
	TypeTraits.h
	UTF8Utils.h
	Utils.h
	ValueAnimator.h
	XMLFactory.h
	XMLObjectParser.h
)
