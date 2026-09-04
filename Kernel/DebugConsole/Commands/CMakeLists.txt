target_sources(PadOS_KDebugConsole PRIVATE
	cat.cpp
	cd.cpp
	echo.cpp
	exit.cpp
	help.cpp
	jobs.cpp
	kill.cpp
	ps.cpp
	reboot.cpp
	time.cpp
)

if(PADOS_MODULE_GPROF_SAMPLING)
	target_sources(PadOS_KDebugConsole PRIVATE
		profile.cpp
	)
endif()
