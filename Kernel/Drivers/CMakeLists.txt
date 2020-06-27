target_sources(PadOS PRIVATE
#	BME280Driver.cpp
#	BME280Driver.h
	FT5x0xDriver.cpp
	FT5x0xDriver.h
	GSLx680Driver.cpp
	GSLx680Driver.h
	GSLx680Firmware.cpp
#	I2CDriver.cpp
#	I2CDriver.h
#	INA3221Driver.cpp
#	INA3221Driver.h
#	UART.cpp
#	UART.h
#	UARTDriver.cpp
#	UARTDriver.h
)

#add_subdirectory(ATSAM)
add_subdirectory(RA8875Driver)
add_subdirectory(SDMMCDriver)
add_subdirectory(STM32)
