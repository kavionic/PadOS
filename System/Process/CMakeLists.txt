target_sources(PadOS_System PRIVATE
	Exit.cpp
)

if(PADOS_MODULE_POSIX_SPAWN)
	target_sources(PadOS_System PRIVATE
		Process.cpp
	)
endif()
