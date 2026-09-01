target_sources(PadOS_Kernel PRIVATE
	FileIO.cpp
	KBlockCache.cpp
	KDirectoryCache.cpp
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

if(PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS)
	target_sources(PadOS_Kernel PRIVATE KBlockCacheDiagnostics.cpp)
endif()

target_sources(PadOS_KernelExt PRIVATE
	KPipeFilesystem.cpp
	KPipeFilesystem.h
	Kpty.cpp
	Kpty.h
)
