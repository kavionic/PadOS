target_sources(PadOS_Syscalls PRIVATE
	SysConditionVar.cpp
	SysDigitalPin.cpp
	SysFilesystem.cpp
	SysLogging.cpp
	SysMessagePort.cpp
	SysMiscSyscalls.cpp
	SysMutex.cpp
	SysObjectWaitGroup.cpp
	SysProcess.cpp
	SysSemaphore.cpp
	SysSignals.cpp
	SysThreads.cpp
	SysTime.cpp
)

if(PADOS_MODULE_POSIX_SPAWN)
	target_sources(PadOS_Syscalls PRIVATE
		SysPosixSpawn.cpp
	)
endif()
