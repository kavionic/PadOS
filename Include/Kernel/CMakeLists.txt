target_sources(PadOS PRIVATE 
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
	Scheduler.h
	SpinTimer.h
)

add_subdirectory(Drivers)
add_subdirectory(FSDrivers)
add_subdirectory(HAL)
add_subdirectory(VFS)
