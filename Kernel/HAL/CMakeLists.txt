target_sources(PadOS_Kernel PRIVATE
	DigitalPort.cpp
#	SAME70System.cpp
)

add_subdirectory(ARM)
add_subdirectory(ATSAM)
add_subdirectory(STM32)
