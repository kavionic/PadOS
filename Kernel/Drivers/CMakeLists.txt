target_sources(PadOS_Kernel PRIVATE
#	BME280Driver.cpp
	FT5x0xDriver.cpp
	GSLx680Driver.cpp
	GSLx680Firmware.cpp
#	I2CDriver.cpp
#	INA3221Driver.cpp
#	UART.cpp
#	UARTDriver.cpp
)

#add_subdirectory(ATSAM)
add_subdirectory(SDMMCDriver)
add_subdirectory(STM32)
