# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Kernel PRIVATE 
	IRQDispatcher.h
	KAddressValidation.h
	KCapabilities.h
	KConditionVariable.h
	Kernel.h
	KHandleArray.h
	KInterrupts.h
	KIRQGuard.h
	KIRQPriorityLevels.h
	KLogging.h
	KMessagePort.h
	KMutex.h
	KNamedObject.h
	KObjectWaitGroup.h
	KPIDNode.h
	KPosixSignals.h
	KPosixSpawn.h
	KPowerManager.h
	KProcess.h
	KProcessGroup.h
	KProcessGroups.h
	KProcessSession.h
	KSchedulerLock.h
	KSemaphore.h
	KThread.h
	KThreadCB.h
	KThreadWaitNode.h
	KTime.h
	KUserspaceService.h
	KWaitableObject.h
	Misc.h
	Scheduler.h
	SpinTimer.h
	Syscalls.h
	ThreadSyncDebugTracker.h
)

add_subdirectory(DebugConsole)
add_subdirectory(Drivers)
add_subdirectory(FSDrivers)
add_subdirectory(HAL)
add_subdirectory(Logging)
add_subdirectory(Profiler)
add_subdirectory(Startup)
add_subdirectory(USB)
add_subdirectory(UserInput)
add_subdirectory(VFS)
