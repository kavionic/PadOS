target_sources(PadOS_Drivers_USB PRIVATE
	USBClientControl.cpp
	USBCommon.cpp
	USBDevice.cpp
)

if(PADOS_MODULE_USB_HOST)
	target_sources(PadOS_Drivers_USB PRIVATE
	USBHost.cpp
	USBHostControl.cpp
	USBHostEnumerator.cpp
	USBHostHub.cpp
	)
	add_subdirectory(DevFS)
endif()


add_subdirectory(ClassDrivers)
