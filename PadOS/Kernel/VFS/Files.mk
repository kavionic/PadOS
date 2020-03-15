MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = FileIO.cpp KBlockCache.cpp KDeviceNode.cpp KFileHandle.cpp KFilesystem.cpp KFSVolume.cpp KINode.cpp KIOContext.cpp KNodeMonitor.cpp KRootFilesystem.cpp KVFSManager.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
