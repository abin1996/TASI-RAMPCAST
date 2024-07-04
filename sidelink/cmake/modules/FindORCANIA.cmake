# - Try to find ORCANIA
# Once done this will define
#  ORCANIA_FOUND        - System has ORCANIA
#  ORCANIA_INCLUDE_DIRS - The ORCANIA include directories
#  ORCANIA_LIBRARIES    - The ORCANIA library

find_package(PkgConfig)
pkg_check_modules(PC_ORCANIA QUIET ORCANIA)
IF(NOT ORCANIA_FOUND)

FIND_PATH(
    ORCANIA_INCLUDE_DIRS
    NAMES orcania.h
    HINTS ${PC_ORCANIA_INCLUDEDIR}
          ${PC_ORCANIA_INCLUDE_DIRS}
          $ENV{ORCANIA_DIR}/include
    PATHS /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    ORCANIA_LIBRARIES
    NAMES orcania
    HINTS ${PC_ORCANIA_LIBDIR}
          ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          $ENV{ORCANIA_DIR}/lib
    PATHS /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

message(STATUS "ORCANIA LIBRARIES " ${ORCANIA_LIBRARIES})
message(STATUS "ORCANIA INCLUDE DIRS " ${ORCANIA_INCLUDE_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ORCANIA DEFAULT_MSG ORCANIA_LIBRARIES ORCANIA_INCLUDE_DIRS)
MARK_AS_ADVANCED(ORCANIA_LIBRARIES ORCANIA_INCLUDE_DIRS)

ENDIF(NOT ORCANIA_FOUND)