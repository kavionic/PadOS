# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
	EventHandler.cpp
	EventTimer.cpp
	Looper.cpp
	Semaphore.cpp
	ThreadLocal.cpp
)

target_sources(PadOS_SystemIFlash PRIVATE
	Thread.cpp
	ThreadUserspaceState.cpp
)
