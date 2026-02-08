target_sources(PadOS_KernelExt PRIVATE
	VirtualFSBase.cpp
)

add_subdirectory(BinFS)
add_subdirectory(FAT)
