#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "avcpp::avcpp" for configuration "Debug"
set_property(TARGET avcpp::avcpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(avcpp::avcpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/libavcpp.a"
  )

list(APPEND _cmake_import_check_targets avcpp::avcpp )
list(APPEND _cmake_import_check_files_for_avcpp::avcpp "${_IMPORT_PREFIX}/debug/lib/libavcpp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
