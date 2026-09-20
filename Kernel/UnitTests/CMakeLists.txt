
target_sources(PadOS_Kernel_Unconditional PRIVATE
	USBHIDReportParser_unittest.cpp
)

if(PADOS_DRIVER_USB)
    target_sources(PadOS_Kernel_Unconditional PRIVATE
        USBClientCDCRecovery_unittest.cpp
    )
endif()
