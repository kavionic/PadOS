MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = LineSegment.cpp Rect.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
