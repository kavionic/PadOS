MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = ApplicationServer.cpp ServerApplication.cpp ServerView.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
