
target_sources(PadOS_Drivers_Misc PRIVATE
	SerialDebugStreamDriver.cpp
)

target_sources(PadOS_KernelExt PRIVATE
	CommandHandlerFilesystem.cpp
	SerialCommandHandler.cpp
)
