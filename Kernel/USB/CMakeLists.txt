target_sources(PadOS_Drivers_USB PRIVATE
	USBClientControl.cpp
	USBCommon.cpp
	USBDevice.cpp
	USBHost.cpp
	USBHostControl.cpp
	USBHostEnumerator.cpp
)


add_subdirectory(ClassDrivers)
