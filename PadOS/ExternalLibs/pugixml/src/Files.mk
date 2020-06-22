MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = pugixml.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
