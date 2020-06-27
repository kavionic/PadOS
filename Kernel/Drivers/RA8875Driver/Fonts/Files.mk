MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = MicrosoftSansSerif_14.c MicrosoftSansSerif_20.c MicrosoftSansSerif_72.c

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
