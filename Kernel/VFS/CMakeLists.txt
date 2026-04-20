target_sources(PadOS_Kernel PRIVATE
	FileIO.cpp
	KBlockCache.cpp
	KDriverManager.cpp
	KFileHandle.cpp
	KFilesystem.cpp
	KFSVolume.cpp
	KInode.cpp
	KIOContext.cpp
	KNodeMonitor.cpp
	KRootFilesystem.cpp
	KVFSManager.cpp
)

target_sources(PadOS_KernelExt PRIVATE
	KPipeFilesystem.cpp
	KPipeFilesystem.h
	Kpty.cpp
	Kpty.h
)
