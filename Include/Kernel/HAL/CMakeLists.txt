target_sources(PadOS PRIVATE 
	DigitalPort.h
	DigitalPort_ATSAM.h
	DigitalPort_STM32.h
	DMA.h
	Peripherals.h
	SAME70System.h
	STM32H7System.h
)

add_subdirectory(ARM)
add_subdirectory(ATSAM)
add_subdirectory(STM32)
