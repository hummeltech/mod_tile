# - Find APR
# Find the APR includes and libraries.
# This module defines:
#  APR_FOUND
#  APR_INCLUDE_DIRS
#  APR_LIBRARIES

find_package(PkgConfig QUIET)
pkg_check_modules(APR QUIET apr-1)

find_path(APR_INCLUDE_DIR
  NAMES apr.h
  PATHS ${APR_INCLUDE_DIRS}
  PATH_SUFFIXES apr-1
)

if((NOT APR_INCLUDE_DIRS) AND (APR_INCLUDE_DIR))
  set(APR_INCLUDE_DIRS ${APR_INCLUDE_DIR})
elseif(APR_INCLUDE_DIRS AND APR_INCLUDE_DIR)
  list(APPEND APR_INCLUDE_DIRS ${APR_INCLUDE_DIR})
endif()

find_library(APR_LIBRARY
  NAMES ${APR_LIBRARIES} apr-1
)

if((NOT APR_LIBRARIES) AND (APR_LIBRARY))
  set(APR_LIBRARIES ${APR_LIBRARY})
elseif(APR_LIBRARIES AND APR_LIBRARY)
  list(APPEND APR_LIBRARIES ${APR_LIBRARY})
endif()

message(VERBOSE "APR_INCLUDE_DIRS=${APR_INCLUDE_DIRS}")
message(VERBOSE "APR_INCLUDE_DIR=${APR_INCLUDE_DIR}")
message(VERBOSE "APR_LIBRARIES=${APR_LIBRARIES}")
message(VERBOSE "APR_LIBRARY=${APR_LIBRARY}")

if((NOT APR_FOUND) AND (APR_INCLUDE_DIRS) AND (APR_LIBRARIES))
  set(APR_FOUND True)
endif()

if((NOT APR_VERSION) AND (APR_FOUND))
  file(STRINGS "${APR_INCLUDE_DIR}/apr_version.h" _contents REGEX "#define APR_[A-Z]+_VERSION[ \t]+")
  if(_contents)
    string(REGEX REPLACE ".*#define APR_MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MAJOR_VERSION "${_contents}")
    string(REGEX REPLACE ".*#define APR_MINOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MINOR_VERSION "${_contents}")
    string(REGEX REPLACE ".*#define APR_PATCH_VERSION[ \t]+([0-9]+).*" "\\1" APR_PATCH_VERSION "${_contents}")

    set(APR_VERSION ${APR_MAJOR_VERSION}.${APR_MINOR_VERSION}.${APR_PATCH_VERSION})
  endif()
endif()

include(FindPackageHandleStandardArgs)

if(APR_FOUND)
  find_package_handle_standard_args(APR
    REQUIRED_VARS APR_FOUND APR_INCLUDE_DIRS APR_LIBRARIES
    VERSION_VAR APR_VERSION
  )
else()
  find_package_handle_standard_args(APR
    REQUIRED_VARS APR_FOUND
  )
endif()

mark_as_advanced(APR_INCLUDE_DIR APR_LIBRARY)
