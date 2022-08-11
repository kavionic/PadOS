target_sources(PadOS_Kernel PRIVATE
	USBClientControl.cpp
	USBCommon.cpp
	USBDevice.cpp
	USBHost.cpp
	USBHostControl.cpp
	USBHostEnumerator.cpp
)


add_subdirectory(ClassDrivers)
