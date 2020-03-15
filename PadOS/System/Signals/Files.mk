MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = SignalBase.cpp SignalSystem.cpp SignalTarget.cpp SlotBase.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
