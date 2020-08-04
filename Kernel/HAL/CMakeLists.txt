target_sources(PadOS PRIVATE
	DigitalPort.cpp
#	SAME70System.cpp
)

add_subdirectory(ARM)
add_subdirectory(ATSAM)
add_subdirectory(STM32)
