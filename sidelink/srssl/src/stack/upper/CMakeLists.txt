#
# Copyright 2013-2019 Software Radio Systems Limited
#
# This file is part of srsLTE
#
# srsLTE is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsLTE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

set(SOURCES gw.cc nas.cc usim_base.cc usim.cc tft_packet_filter.cc)

if(HAVE_PCSC)
  list(APPEND SOURCES "pcsc_usim.cc")
endif(HAVE_PCSC)

if(ENABLE_REST)
  list(APPEND SOURCES "rest.cc")
  message(STATUS "Added rest.c to source: ${SOURCES}")

  include_directories(${ULFIUS_INCLUDE_DIRS})
  include_directories(${JANSSON_INCLUDE_DIRS})
endif(ENABLE_REST)

add_library(srssl_upper STATIC ${SOURCES})

if(HAVE_PCSC)
  target_link_libraries(srssl_upper ${PCSCLITE_LIBRARY})
endif(HAVE_PCSC)

if(ENABLE_REST)
  target_link_libraries(srssl_upper ${ULFIUS_LIBRARIES} ${JANSSON_LIBRARIES})
endif(ENABLE_REST)

install(TARGETS srssl_upper DESTINATION ${LIBRARY_DIR})
