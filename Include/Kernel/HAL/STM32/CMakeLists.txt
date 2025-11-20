target_sources(PadOS_Kernel PRIVATE
	DMA_STM32.h
	DMARequestID.h
	PeripheralMapping_STM32G030xx.h
	PeripheralMapping_STM32H7.h
	Peripherals_STM32H7.h
	PiezoBuzzer_STM32.h
	PinMuxTarget_STM32.h
	PinMuxTarget_STM32G030xx.h
	PinMuxTarget_STM32H7.h
	RealtimeClock.h
	ResetAndClockControl.h
)

if(PADOS_DRIVER_ADC)
	target_sources(PadOS_Drivers_ADC PRIVATE
		ADC_STM32.h
	)
endif()

if(PADOS_DRIVER_QSPI)
	target_sources(PadOS_Drivers_QSPI PRIVATE
		QSPI_STM32.h
		QSPI_STM32_IS25LP512M.h
	)
endif()

if(PADOS_DRIVER_SDRAM)
	target_sources(PadOS_Drivers_SDRAM PRIVATE
		SDRAM_STM32.h
	)
endif()

if(PADOS_DRIVER_USB)
	target_sources(PadOS_Drivers_USB PRIVATE
		USB_STM32.h
		USBDevice_STM32.h
		USBHost_STM32.h
	)
endif()

if(PADOS_DRIVER_RealtimeClock)
	target_sources(PadOS_Drivers_RealtimeClock PRIVATE
		RealtimeClock.h
	)
endif()
