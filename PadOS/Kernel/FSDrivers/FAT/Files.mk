MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = FATClusterSectorIterator.cpp FATDirectoryIterator.cpp FATDirectoryNode.cpp FATFileNode.cpp FATFilesystem.cpp FATINode.cpp FATTable.cpp FATTableIterator.cpp FATVolume.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
