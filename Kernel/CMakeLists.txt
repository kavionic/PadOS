add_subdirectory(Drivers)
add_subdirectory(FSDrivers)
add_subdirectory(HAL)
add_subdirectory(VFS)

target_sources(PadOS_Kernel PRIVATE
	IRQDispatcher.cpp
	KConditionVariable.cpp
	Kernel.cpp
	KHandleArray.cpp
	KMessagePort.cpp
	KMutex.cpp
	KNamedObject.cpp
	KObjectWaitGroup.cpp
	KPowerManager.cpp
	KProcess.cpp
	KSemaphore.cpp
	KThreadCB.cpp
	Scheduler.cpp
	SpinTimer.cpp
	SysTime.cpp
)
