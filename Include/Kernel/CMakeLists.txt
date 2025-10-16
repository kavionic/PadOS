target_sources(PadOS_Kernel PRIVATE 
	IRQDispatcher.h
	KConditionVariable.h
	Kernel.h
	KHandleArray.h
	KMessagePort.h
	KMutex.h
	KNamedObject.h
	KObjectWaitGroup.h
	KPowerManager.h
	KProcess.h
	KSemaphore.h
	KThreadCB.h
	KThreadWaitNode.h
	KTime.h
	KWaitableObject.h
	Misc.h
	Scheduler.h
	SpinTimer.h
	Syscalls.h
	ThreadSyncDebugTracker.h
)

add_subdirectory(Drivers)
add_subdirectory(FSDrivers)
add_subdirectory(HAL)
add_subdirectory(USB)
add_subdirectory(VFS)
