MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = FT5x0xDriver.cpp GSLx680Driver.cpp GSLx680Firmware.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
