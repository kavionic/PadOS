target_sources(PadOS_Kernel PRIVATE
	BME280Driver.h
	FT5x0xDriver.h
	GSLx680Driver.h
	I2CDriver.h
	INA3221Driver.h
	UART.h
	UARTDriver.h
)

#add_subdirectory(ATSAM)
if(PADOS_DRIVER_MultiMotor)
	add_subdirectory(MultiMotorController)
endif()
if(PADOS_DRIVER_SDMMC)
	add_subdirectory(SDMMCDriver)
endif()
add_subdirectory(STM32)
