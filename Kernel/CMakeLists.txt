if(PADOS_MODULE_DEBUG_CONSOLE)
	add_subdirectory(DebugConsole)
endif()
add_subdirectory(Drivers)
add_subdirectory(FSDrivers)
add_subdirectory(HAL)
add_subdirectory(Interrupts)
add_subdirectory(Logging)
add_subdirectory(Startup)
add_subdirectory(Syscalls)
add_subdirectory(USB)
add_subdirectory(VFS)

target_sources(PadOS_Kernel PRIVATE
	IRQDispatcher.cpp
	KCapabilities.cpp
	KConditionVariable.cpp
	Kernel.cpp
	KHandleArray.cpp
	KMessagePort.cpp
	KMutex.cpp
	KNamedObject.cpp
	KObjectWaitGroup.cpp
	KPIDNode.cpp
	KPowerManager.cpp
	KProcess.cpp
	KProcessGroup.cpp
	KProcessGroups.cpp
	KProcessSession.cpp
	KSemaphore.cpp
	KThread.cpp
	KThreadCB.cpp
	KTime.cpp
	KWaitableObject.cpp
	Misc.cpp
	Scheduler.cpp
	SpinTimer.cpp
	Syscalls.cpp
	ThreadSyncDebugTracker.cpp
)

target_sources(PadOS_KernelExt PRIVATE
	KThreadExitTrampoline.cpp
)

if(PADOS_MODULE_POSIX_SIGNALS)
	target_sources(PadOS_KernelExt PRIVATE
		KPosixSignals.cpp
	)
endif()

if(PADOS_MODULE_POSIX_SPAWN)
	target_sources(PadOS_KernelExt PRIVATE
		KPosixSpawn.cpp
	)
endif()
