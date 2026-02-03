target_sources(PadOS_Kernel PRIVATE
	VirtualFSBase.h
)

add_subdirectory(BinFS)
add_subdirectory(FAT)
