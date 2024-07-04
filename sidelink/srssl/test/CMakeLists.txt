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

# currently we do not test upper layers in srssl
# add_subdirectory(upper)

add_executable(metrics_test_sl metrics_test.cc ../src/metrics_stdout.cc ../src/metrics_csv.cc)
target_link_libraries(metrics_test_sl srslte_phy srslte_common)
add_test(metrics_test_sl metrics_test_sl -o ${CMAKE_CURRENT_BINARY_DIR}/ue_metrics.csv)

add_executable(mac_test_sl mac_test.cc)
target_link_libraries(mac_test_sl srssl_mac srssl_phy srslte_common srslte_phy srslte_radio srslte_asn1 rrc_asn1 ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_test(mac_test_sl mac_test_sl)

add_executable(sps_test_sl sps_test.cc)
target_link_libraries(sps_test_sl srssl_mac srssl_phy srslte_common srslte_phy srslte_radio srslte_asn1 rrc_asn1 ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_test(sps_test_sl sps_test_sl)

if(ENABLE_REST)
    add_executable(rest_test rest_test.cc)
    target_link_libraries(rest_test srssl_upper srssl_phy srslte_common rrc_asn1 ${ORCANIA_LIBRARIES} ${ULFIUS_LIBRARIES} ${JANSSON_LIBRARIES})
    add_test(rest_test rest_test)
endif(ENABLE_REST)
