# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
	ConditionVariable.h 
	EventHandler.h
	EventTimer.h
	Looper.h
	Mutex.h
	ObjectWaitGroup.h
	Semaphore.h
	Thread.h
	ThreadLocal.h
	Threads.h
	ThreadUserspaceState.h
)
