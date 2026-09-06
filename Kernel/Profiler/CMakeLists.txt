if(PADOS_MODULE_GPROF_SAMPLING)
	target_sources(PadOS_Kernel PRIVATE
		KGProfData.h
		KGProfSampler.cpp
	)

	target_sources(PadOS_KernelExt PRIVATE
		KGProf.cpp
	)
endif()
