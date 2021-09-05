set(ODE_VERSION "0.16.0")
set(ODE_VERSION_MAJOR "0")
set(ODE_VERSION_MINOR "16")
set(ODE_VERSION_PATCH "0")

set(ODE_16BIT_INDICES ON)
set(ODE_DOUBLE_PRECISION OFF)
set(ODE_NO_BUILTIN_THREADING_IMPL OFF)
set(ODE_NO_THREADING_INTF OFF)
set(ODE_OLD_TRIMESH OFF)
set(ODE_WITH_GIMPACT OFF)
set(ODE_WITH_LIBCCD OFF)
set(ODE_WITH_LIBCCD_BOX_CYL OFF)
set(ODE_WITH_LIBCCD_CAP_CYL OFF)
set(ODE_WITH_LIBCCD_CYL_CYL OFF)
set(ODE_WITH_LIBCCD_CONVEX_BOX OFF)
set(ODE_WITH_LIBCCD_CONVEX_CAP OFF)
set(ODE_WITH_LIBCCD_CONVEX_CONVEX OFF)
set(ODE_WITH_LIBCCD_CONVEX_CYL OFF)
set(ODE_WITH_LIBCCD_CONVEX_SPHERE OFF)
set(ODE_WITH_LIBCCD_SYSTEM OFF)
set(ODE_WITH_OPCODE ON)
set(ODE_WITH_OU ON)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was ode-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/ode-export.cmake")

set(ODE_DEFINITIONS "")
set(ODE_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include")
set(ODE_LIBRARY_DIR "${PACKAGE_PREFIX_DIR}/lib")

macro(select_library_location target basename)
	if(TARGET ${target})
		foreach(property IN ITEMS IMPORTED_LOCATION IMPORTED_IMPLIB)
			get_target_property(${basename}_${property}_DEBUG ${target} ${property}_DEBUG)
			get_target_property(${basename}_${property}_MINSIZEREL ${target} ${property}_MINSIZEREL)
			get_target_property(${basename}_${property}_RELEASE ${target} ${property}_RELEASE)
			get_target_property(${basename}_${property}_RELWITHDEBINFO ${target} ${property}_RELWITHDEBINFO)
			
			if(${basename}_${property}_DEBUG AND ${basename}_${property}_RELEASE)
				set(${basename}_LIBRARY debug ${${basename}_${property}_DEBUG} optimized ${${basename}_${property}_RELEASE})
			elseif(${basename}_${property}_DEBUG AND ${basename}_${property}_RELWITHDEBINFO)
				set(${basename}_LIBRARY debug ${${basename}_${property}_DEBUG} optimized ${${basename}_${property}_RELWITHDEBINFO})
			elseif(${basename}_${property}_DEBUG AND ${basename}_${property}_MINSIZEREL)
				set(${basename}_LIBRARY debug ${${basename}_${property}_DEBUG} optimized ${${basename}_${property}_MINSIZEREL})
			elseif(${basename}_${property}_RELEASE)
				set(${basename}_LIBRARY ${${basename}_${property}_RELEASE})
			elseif(${basename}_${property}_RELWITHDEBINFO)
				set(${basename}_LIBRARY ${${basename}_${property}_RELWITHDEBINFO})
			elseif(${basename}_${property}_MINSIZEREL)
				set(${basename}_LIBRARY ${${basename}_${property}_MINSIZEREL})
			elseif(${basename}_${property}_DEBUG)
				set(${basename}_LIBRARY ${${basename}_${property}_DEBUG})
			endif()
		endforeach()
	endif()
endmacro()

select_library_location(ODE::ODE ODE)

set(ODE_INCLUDE_DIRS ${ODE_INCLUDE_DIR})
set(ODE_LIBRARIES ${ODE_LIBRARY})
set(ODE_LIBRARY_DIRS ${ODE_LIBRARY_DIR})

include(CMakeFindDependencyMacro)

if(ODE_WITH_LIBCCD_SYSTEM)
	find_dependency(ccd)
	list(APPEND ODE_LIBRARIES ${CCD_LIBRARIES})
	list(APPEND ODE_LIBRARY_DIRS ${CCD_LIBRARY_DIRS})
endif()
