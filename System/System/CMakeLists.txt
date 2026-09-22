target_sources(PadOS_System PRIVATE
	AppDefinition.cpp
)

if(PADOS_MODULE_GPROF_CALL_GRAPH)
	target_sources(PadOS_SystemIFlash PRIVATE GProfCallGraph.cpp GProfMCount.cpp)
	set_source_files_properties(GProfCallGraph.cpp GProfMCount.cpp TARGET_DIRECTORY PadOS_SystemIFlash PROPERTIES
		COMPILE_OPTIONS "-fno-profile"
		SKIP_UNITY_BUILD_INCLUSION ON
		SKIP_PRECOMPILE_HEADERS ON
	)
	set_property(SOURCE GProfCallGraph.cpp TARGET_DIRECTORY PadOS_SystemIFlash APPEND PROPERTY COMPILE_OPTIONS -mgeneral-regs-only)
endif()

if(PADOS_MODULE_USER_SPACE)
	target_sources(PadOS_System PRIVATE UserspaceService.cpp)
endif()
