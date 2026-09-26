# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_KDebugConsole PRIVATE
	KConsoleCommand.cpp
	KDebugConsole.cpp
	KSerialMux.cpp
	KSerialMux.h
	KSerialPseudoTerminal.cpp
	KSerialPseudoTerminal.h
)

add_subdirectory(Commands)
