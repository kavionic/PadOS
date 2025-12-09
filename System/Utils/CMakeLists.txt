target_sources(PadOS_System PRIVATE
	Base64Codec.cpp
	Beep.cpp
	FactoryAutoRegistrator.cpp
	HashCalculator.cpp
	InertialScroller.cpp
	IntrusiveList.cpp
	MessagePort.cpp
	String.cpp
	Utils.cpp
	XMLFactory.cpp
	XMLObjectParser.cpp
)

target_sources(PadOS_SystemIFlash PRIVATE
	RelocMemSection.cpp
)
