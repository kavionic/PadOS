target_sources(PadOS PRIVATE
	DigitalPort.cpp
	DigitalPort.h
	DigitalPort_ATSAM.h
	DigitalPort_STM32.h
	DMA.h
	Peripherals.h
#	SAME70System.cpp
#	SAME70System.h
	STM32H7System.h
)

add_subdirectory(STM32)
