MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = Application.cpp Button.cpp Font.cpp LayoutNode.cpp Region.cpp TextView.cpp View.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
