if(PADOS_MODULE_GPROF_SAMPLING)
	target_sources(PadOS_Kernel PRIVATE
		KGProfData.h
		KGProfSampler.cpp
	)

	target_sources(PadOS_KernelExt PRIVATE
		KGProf.cpp
	)
endif()

if(PADOS_MODULE_GPROF_CALL_GRAPH)
	set_source_files_properties(KGProf.cpp KGProfSampler.cpp TARGET_DIRECTORY PadOS_Kernel PadOS_KernelExt PROPERTIES
		COMPILE_OPTIONS "-fno-profile"
		SKIP_UNITY_BUILD_INCLUSION ON
		SKIP_PRECOMPILE_HEADERS ON
	)
endif()
