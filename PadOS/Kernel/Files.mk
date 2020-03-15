
MOD_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

MOD_SOURCES = KConditionVariable.cpp Kernel.cpp KHandleArray.cpp KMessagePort.cpp KMutex.cpp KNamedObject.cpp KObjectWaitGroup.cpp KPowerManager.cpp KProcess.cpp KSemaphore.cpp KThreadCB.cpp Scheduler.cpp SpinTimer.cpp

SOURCES += $(addprefix $(MOD_PATH), $(MOD_SOURCES))
