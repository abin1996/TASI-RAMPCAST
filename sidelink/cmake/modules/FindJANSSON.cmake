# - Try to find JANSSON
# Once done this will define
#  JANSSON_FOUND        - System has jansson
#  JANSSON_INCLUDE_DIRS - The jansson include directories
#  JANSSON_LIBRARIES    - The jansson library

find_package(PkgConfig)
pkg_check_modules(PC_JANSSON QUIET jansson)
IF(NOT JANSSON_FOUND)

FIND_PATH(
    JANSSON_INCLUDE_DIRS
    NAMES jansson.h
    HINTS ${PC_JANSSON_INCLUDEDIR}
          ${PC_JANSSON_INCLUDE_DIRS}
          $ENV{JANSSON_DIR}/include
    PATHS /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    JANSSON_LIBRARIES
    NAMES jansson
    HINTS ${PC_JANSSON_LIBDIR}
          ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          $ENV{JANSSON_DIR}/lib
    PATHS /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

message(STATUS "JANSSON LIBRARIES " ${JANSSON_LIBRARIES})
message(STATUS "JANSSON INCLUDE DIRS " ${JANSSON_INCLUDE_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JANSSON DEFAULT_MSG JANSSON_LIBRARIES JANSSON_INCLUDE_DIRS)
MARK_AS_ADVANCED(JANSSON_LIBRARIES JANSSON_INCLUDE_DIRS)

ENDIF(NOT JANSSON_FOUND)

