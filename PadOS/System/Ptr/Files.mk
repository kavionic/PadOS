MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = PtrTarget.cpp 

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
