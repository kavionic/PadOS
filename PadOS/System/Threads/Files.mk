MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = EventHandler.cpp Looper.cpp Semaphore.cpp Thread.cpp ThreadLocal.cpp 

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
