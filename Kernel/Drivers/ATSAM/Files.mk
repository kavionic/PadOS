MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = SDMMC_ATSAM.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
