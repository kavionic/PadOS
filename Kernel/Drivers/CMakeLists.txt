
if(PADOS_DRIVER_FT5x0x)
target_sources(PadOS_Drivers_FT5x0x PRIVATE
	FT5x0xDriver.cpp
)
endif()

if(PADOS_DRIVER_GSLx680)
target_sources(PadOS_Drivers_GSLx680 PRIVATE
	GSLx680Driver.cpp
	GSLx680Firmware.cpp
)
endif()

target_sources(PadOS_Kernel PRIVATE
#	BME280Driver.cpp
#	I2CDriver.cpp
#	INA3221Driver.cpp
#	UART.cpp
#	UARTDriver.cpp
)

#add_subdirectory(ATSAM)
if(PADOS_DRIVER_MultiMotor)
	add_subdirectory(MultiMotorController)
endif()
if(PADOS_DRIVER_SDMMC)
	add_subdirectory(SDMMCDriver)
endif()
add_subdirectory(STM32)
