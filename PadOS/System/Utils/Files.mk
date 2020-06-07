MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = IntrusiveList.cpp MessagePort.cpp String.cpp Utils.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
