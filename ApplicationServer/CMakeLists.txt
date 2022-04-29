target_sources(PadOS_ApplicationServer PRIVATE
	ApplicationServer.cpp
	DisplayDriver.cpp
	DisplayDriverLineRender.cpp
	ServerApplication.cpp
	ServerApplication.h
	ServerBitmap.cpp
	ServerView.cpp
	ServerView.h
)

add_subdirectory(Drivers)
add_subdirectory(Fonts)
