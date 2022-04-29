target_sources(PadOS_ApplicationServer PRIVATE
	ApplicationServer.h
	DisplayDriver.h
	Font.h
	Protocol.h
	ServerBitmap.h
)

add_subdirectory(Drivers)
add_subdirectory(Fonts)
