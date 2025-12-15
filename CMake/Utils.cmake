
# Try to find an external PadOS build config file. Priority:
#  1) PADOS_CONFIG_FILE (may be absolute or relative to PadOS source)
#  2) ../pados_config.cmake next to PadOS (default)

function(pados_find_build_config outConfigPath)
	set(configCandidates "")

	if(PADOS_CONFIG_FILE)
	    list(APPEND configCandidates "${PADOS_CONFIG_FILE}")
	endif()

	list(APPEND configCandidates "${CMAKE_SOURCE_DIR}/../pados_config.cmake")

	foreach(configFile IN LISTS configCandidates)
		if(NOT configFile)
			continue()
		endif()

		get_filename_component(configFile "${configFile}" ABSOLUTE "${CMAKE_SOURCE_DIR}")

		if(EXISTS "${configFile}")
			set(${outConfigPath} "${configFile}")
			return(PROPAGATE ${outConfigPath})
		endif()
	endforeach()
	unset(${outVarName})
	return(PROPAGATE ${outConfigPath})
endfunction()

# Usage:
#   set_libraries_section(".iflash" Library1 Library2 ...)
function(set_libraries_section SUFFIX)
    foreach(tgt IN LISTS ARGN)
        if (NOT TARGET ${tgt})
            message(FATAL_ERROR "set_libraries_section: '${tgt}' is not a target.")
        endif()

        # Get current OUTPUT_NAME if set, otherwise fall back to target name.
        get_target_property(cur_name ${tgt} OUTPUT_NAME)
        if (NOT cur_name OR cur_name STREQUAL "cur_name-NOTFOUND")
            set(cur_name "${tgt}")
        endif()
	message(STATUS "Rename ${tgt} to ${cur_name}.${SUFFIX}")
        # Append the suffix.
        set_target_properties(${tgt} PROPERTIES OUTPUT_NAME "${cur_name}.${SUFFIX}")
    endforeach()
endfunction()

function(setup_unity_build_and_pch MIN_CXX_SOURCES PADOS_PCH_HEADER)
	get_property(all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
	foreach(buildTarget ${all_targets})
		# Skip imported targets (toolchain, external stuff)
		get_target_property(is_imported ${buildTarget} IMPORTED)
		if(is_imported)
			continue()
		endif()

		# Skip pure INTERFACE libraries
		get_target_property(type ${buildTarget} TYPE)
		if(type STREQUAL "INTERFACE_LIBRARY")
			continue()
		endif()

		# Get the sources for this target
		get_target_property(targetSources ${buildTarget} SOURCES)
		if(NOT targetSources)
			continue()
		endif()

		if(PADOS_OPT_USE_UNITY_BUILD)
			message(STATUS "Enabling unity build for target ${buildTarget}")
			set_target_properties(${buildTarget} PROPERTIES UNITY_BUILD ON)
		endif()

		if(PADOS_OPT_USE_PCH)
			# Count C++ sources
			set(cxx_count 0)
			foreach(src ${targetSources})
				# Ignore generator expressions etc.
				if(src MATCHES "^\\$<")
					continue()
				endif()

				if(src MATCHES "\\.(cxx|cpp|cc|C)$")
					math(EXPR cxx_count "${cxx_count} + 1")
				endif()
			endforeach()

			# Only enable PCH if we have enough C++ files
			if(cxx_count GREATER_EQUAL MIN_CXX_SOURCES)
				message(STATUS "Enabling PCH for target ${buildTarget} (${cxx_count} C++ sources)")
				target_precompile_headers(${buildTarget} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${PADOS_PCH_HEADER}>")
			else()
				message(STATUS "Skipping PCH for target ${buildTarget} (${cxx_count} C++ sources)")
			endif()
		endif()
	endforeach()
endfunction()

set(PADOS_KERNEL_MODULE_TARGETS "")
set(PADOS_KERNEL_LIBRARY_TARGETS "")

set(PADOS_KERNEL_MODULE_FILES "")
set(PADOS_KERNEL_LIBRARY_FILES "")

macro(pados_add_compile_option option define)
    if(${opt})
	add_compile_definitions(${define}=1)
    endif()
endmacro()

macro(pados_add_kernel_module opt lib)
    if(${opt})
	message(STATUS "Enabling kernel module ${lib}")
        list(APPEND PADOS_KERNEL_MODULE_TARGETS ${lib})
	add_library(${lib})

        get_target_property(library_name ${lib} OUTPUT_NAME)
        if (NOT library_name OR library_name STREQUAL "cur_name-NOTFOUND")
            set(library_name "${lib}")
        endif()
        list(APPEND PADOS_KERNEL_MODULE_FILES "PadOS::${lib}")
    endif()
endmacro()

macro(pados_add_kernel_library opt lib)
    if(${opt})
        list(APPEND PADOS_KERNEL_LIBRARY_TARGETS ${lib})
#	add_library(${lib})

        get_target_property(library_name ${lib} OUTPUT_NAME)
        if (NOT library_name OR library_name STREQUAL "cur_name-NOTFOUND")
            set(library_name "${lib}")
        endif()
        list(APPEND PADOS_KERNEL_LIBRARY_FILES "PadOS::${lib}")
    endif()
endmacro()
