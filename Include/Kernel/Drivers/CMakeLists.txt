target_sources(PadOS PRIVATE
	BME280Driver.h
	FT5x0xDriver.h
	GSLx680Driver.h
	I2CDriver.h
	INA3221Driver.h
	UART.h
	UARTDriver.h
)

#add_subdirectory(ATSAM)
add_subdirectory(RA8875Driver)
add_subdirectory(SDMMCDriver)
add_subdirectory(STM32)
