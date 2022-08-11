target_sources(PadOS_Kernel PRIVATE
	USBClassDriverDevice.h
	USBClassDriverHost.h
	USBClientControl.h
	USBCommon.h
	USBDevice.h
	USBDriver.h
	USBEndpointState.h
	USBHost.h
	USBHostControl.h
	USBHostEnumerator.h
	USBProtocol.h
	USBProtocolCDC.h
)

add_subdirectory(ClassDrivers)
