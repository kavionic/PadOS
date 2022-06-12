target_sources(PadOS_Kernel PRIVATE
	USBClassDriver.h
	USBCommon.h
	USBControlTransferHandler.h
	USBDevice.h
	USBDriver.h
	USBProtocol.h
	USBProtocolCDC.h
)

add_subdirectory(ClassDrivers)
