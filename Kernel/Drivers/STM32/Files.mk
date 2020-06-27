MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = I2CDriver.cpp SDMMCDriver_STM32.cpp TLV493DDriver.cpp USARTDriver.cpp WS2812BDriver.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
