MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = I2CDriver.cpp TLV493DDriver.cpp USARTDriver.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
