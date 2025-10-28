target_sources(PadOS_Drivers_FT5x0x PRIVATE
	FT5x0xDriver.cpp
)

target_sources(PadOS_Drivers_GSLx680 PRIVATE
	GSLx680Driver.cpp
	GSLx680Firmware.cpp
)

target_sources(PadOS_Kernel PRIVATE
#	BME280Driver.cpp
#	I2CDriver.cpp
#	INA3221Driver.cpp
#	UART.cpp
#	UARTDriver.cpp
)

#add_subdirectory(ATSAM)
add_subdirectory(MultiMotorController)
add_subdirectory(SDMMCDriver)
add_subdirectory(STM32)
