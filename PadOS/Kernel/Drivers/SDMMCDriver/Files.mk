MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = SDMMCDriver.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
