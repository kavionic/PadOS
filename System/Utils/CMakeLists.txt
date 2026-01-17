target_sources(PadOS_System PRIVATE
	ANSIEscapeCodeParser.cpp
	Base64Codec.cpp
	Beep.cpp
	FactoryAutoRegistrator.cpp
	HashCalculator.cpp
	InertialScroller.cpp
	IntrusiveList.cpp
	MessagePort.cpp
	POSIXTokenizer.cpp
	String.cpp
	Utils.cpp
	XMLFactory.cpp
	XMLObjectParser.cpp
)

target_sources(PadOS_SystemIFlash PRIVATE
	RelocMemSection.cpp
)
