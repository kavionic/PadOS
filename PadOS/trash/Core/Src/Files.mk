MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = system_stm32h7xx.c

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
