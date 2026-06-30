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
	USBHostHub.h
	USBProtocol.h
	USBProtocolCDC.h
	USBProtocolHID.h
	USBProtocolHub.h
)

add_subdirectory(ClassDrivers)
