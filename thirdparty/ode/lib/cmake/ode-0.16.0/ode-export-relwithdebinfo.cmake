#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ODE::ODE" for configuration "RelWithDebInfo"
set_property(TARGET ODE::ODE APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(ODE::ODE PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/ode_single.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/ode_single.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS ODE::ODE )
list(APPEND _IMPORT_CHECK_FILES_FOR_ODE::ODE "${_IMPORT_PREFIX}/lib/ode_single.lib" "${_IMPORT_PREFIX}/bin/ode_single.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
