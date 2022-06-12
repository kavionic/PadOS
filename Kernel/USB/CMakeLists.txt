target_sources(PadOS_Kernel PRIVATE
	USBCommon.cpp
	USBControlTransferHandler.cpp
	USBDevice.cpp
)

add_subdirectory(ClassDrivers)
