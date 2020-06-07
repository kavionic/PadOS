MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = EventHandler.cpp EventTimer.cpp Looper.cpp Semaphore.cpp Thread.cpp ThreadLocal.cpp 

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
