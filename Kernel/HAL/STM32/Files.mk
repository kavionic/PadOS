MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = DMA_STM32.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
