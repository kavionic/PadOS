MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = HashCalculator.cpp IntrusiveList.cpp MessagePort.cpp String.cpp Utils.cpp XMLFactory.cpp XMLObjectParser.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
