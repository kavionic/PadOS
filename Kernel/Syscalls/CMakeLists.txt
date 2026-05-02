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
	SysThreads.cpp
	SysTime.cpp
)

if(PADOS_MODULE_POSIX_SIGNALS)
	target_sources(PadOS_Syscalls PRIVATE
		SysSignals.cpp
	)
endif()

if(PADOS_MODULE_POSIX_SPAWN)
	target_sources(PadOS_Syscalls PRIVATE
		SysPosixSpawn.cpp
	)
endif()
