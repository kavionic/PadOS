MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = EventTimer.cpp IntrusiveList.cpp MessagePort.cpp Utils.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
