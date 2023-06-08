target_sources(PadOS_Kernel PRIVATE
	DigitalPort_STM32.cpp
	DMA_STM32.cpp
	PeripheralMapping_STM32G030xx.cpp
	PeripheralMapping_STM32H7.cpp
	PiezoBuzzer_STM32.cpp
	ResetAndClockControl.cpp
)

target_sources(PadOS_Drivers_ADC PRIVATE
	ADC_STM32.cpp
)

target_sources(PadOS_Drivers_QSPI PRIVATE
	QSPI_STM32.cpp
	QSPI_STM32_IS25LP512M.cpp
)

target_sources(PadOS_Drivers_SDRAM PRIVATE
	SDRAM_STM32.cpp
)

target_sources(PadOS_Drivers_USB PRIVATE
	USB_STM32.cpp
	USBDevice_STM32.cpp
	USBHost_STM32.cpp
)

target_sources(PadOS_Drivers_RealtimeClock PRIVATE
	RealtimeClock.cpp
)
