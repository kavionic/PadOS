MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = Application.cpp Button.cpp Control.cpp Font.cpp GroupView.cpp LayoutNode.cpp Region.cpp Slider.cpp TextView.cpp View.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
