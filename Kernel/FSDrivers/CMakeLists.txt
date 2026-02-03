target_sources(PadOS_Kernel PRIVATE
	VirtualFSBase.cpp
)

add_subdirectory(BinFS)
add_subdirectory(FAT)
