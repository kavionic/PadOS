target_sources(PadOS_Drivers_USB PRIVATE
	USBClientCDCChannel.cpp
	USBClientClassCDC.cpp
)

if(PADOS_MODULE_USB_HOST)
	target_sources(PadOS_Drivers_USB PRIVATE
	USBHostCDCChannel.cpp
	USBHostClassCDC.cpp
	USBHostHIDInterface.cpp
	USBHostClassHID.cpp
	)
endif()
