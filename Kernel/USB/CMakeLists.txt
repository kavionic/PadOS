target_sources(PadOS_Kernel PRIVATE
	USBCommon.cpp
	USBControlTransferHandler.cpp
	USBDevice.cpp
	USBHost.cpp
	USBHostControl.cpp
	USBHostEnumerator.cpp
)


add_subdirectory(ClassDrivers)
