# - Try to find ULFIUS
# Once done this will define
#  ULFIUS_FOUND        - System has ulfius
#  ULFIUS_INCLUDE_DIRS - The ulfius include directories
#  ULFIUS_LIBRARIES    - The ulfius library

find_package(PkgConfig)
pkg_check_modules(PC_ULFIUS QUIET ulfius)
IF(NOT ULFIUS_FOUND)

FIND_PATH(
    ULFIUS_INCLUDE_DIRS
    NAMES ulfius.h
    HINTS ${PC_ULFIUS_INCLUDEDIR}
          ${PC_ULFIUS_INCLUDE_DIRS}
          $ENV{ULFIUS_DIR}/include
    PATHS /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    ULFIUS_LIBRARIES
    NAMES ulfius
    HINTS ${PC_ULFIUS_LIBDIR}
          ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          $ENV{ULFIUS_DIR}/lib
    PATHS /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

message(STATUS "ULFIUS LIBRARIES " ${ULFIUS_LIBRARIES})
message(STATUS "ULFIUS INCLUDE DIRS " ${ULFIUS_INCLUDE_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ULFIUS DEFAULT_MSG ULFIUS_LIBRARIES ULFIUS_INCLUDE_DIRS)
MARK_AS_ADVANCED(ULFIUS_LIBRARIES ULFIUS_INCLUDE_DIRS)

ENDIF(NOT ULFIUS_FOUND)
