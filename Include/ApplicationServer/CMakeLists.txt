target_sources(PadOS PRIVATE
	ApplicationServer.h
	DisplayDriver.h
	Font.h
	Protocol.h
	ServerBitmap.h
)

add_subdirectory(Drivers)
add_subdirectory(Fonts)
