
if(PADOS_DRIVER_I2C)
	target_sources(PadOS_Drivers_I2C PRIVATE
		I2CDriver.cpp
	)
endif()

if(PADOS_DRIVER_SDMMC)
	target_sources(PadOS_Drivers_SDMMC PRIVATE
		SDMMCDriver_STM32.cpp
	)
endif()

if(PADOS_DRIVER_SPI)
	target_sources(PadOS_Drivers_SPI PRIVATE
		SPIDriver.cpp
	)
endif()

if(PADOS_DRIVER_TLV493)
	target_sources(PadOS_Drivers_TLV493 PRIVATE
		TLV493DDriver.cpp
	)
endif()

if(PADOS_DRIVER_USART)
	target_sources(PadOS_Drivers_USART PRIVATE
		USARTDriver.cpp
	)
endif()

if(PADOS_DRIVER_WS2812B)
	target_sources(PadOS_Drivers_WS2812B PRIVATE
		WS2812BDriver.cpp
	)
endif()
