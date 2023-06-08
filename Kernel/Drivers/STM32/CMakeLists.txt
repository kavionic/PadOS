target_sources(PadOS_Drivers_I2C PRIVATE
	I2CDriver.cpp
)

target_sources(PadOS_Drivers_SDMMC PRIVATE
	SDMMCDriver_STM32.cpp
)

target_sources(PadOS_Drivers_TLV493 PRIVATE
	TLV493DDriver.cpp
)

target_sources(PadOS_Drivers_USART PRIVATE
	USARTDriver.cpp
)

target_sources(PadOS_Drivers_WS2812B PRIVATE
	WS2812BDriver.cpp
)
