#
# Copyright 2013-2022 iSignal Research Labs Pvt Ltd.
#
# This file is part of isrRAN
#
# isrRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# isrRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

set(CTEST_LABELS "lib;rlc")

add_executable(rlc_am_data_test rlc_am_data_test.cc)
target_link_libraries(rlc_am_data_test isrran_rlc isrran_phy isrran_common)
add_lte_test(rlc_am_data_test rlc_am_data_test)

add_executable(rlc_am_control_test rlc_am_control_test.cc)
target_link_libraries(rlc_am_control_test isrran_rlc isrran_phy)
add_lte_test(rlc_am_control_test rlc_am_control_test)

add_executable(rlc_am_lte_test rlc_am_lte_test.cc)
target_link_libraries(rlc_am_lte_test isrran_rlc isrran_phy isrran_common)
add_lte_test(rlc_am_lte_test rlc_am_lte_test)

add_executable(rlc_am_nr_test rlc_am_nr_test.cc)
target_link_libraries(rlc_am_nr_test isrran_rlc isrran_phy isrran_common)
add_nr_test(rlc_am_nr_test rlc_am_nr_test)

add_executable(rlc_am_nr_pdu_test rlc_am_nr_pdu_test.cc)
target_link_libraries(rlc_am_nr_pdu_test isrran_rlc isrran_phy isrran_mac isrran_common )
add_nr_test(rlc_am_nr_pdu_test rlc_am_nr_pdu_test )

add_executable(rlc_stress_test rlc_stress_test.cc)
target_link_libraries(rlc_stress_test isrran_rlc isrran_mac isrran_phy isrran_common ${Boost_LIBRARIES} ${ATOMIC_LIBS})
add_lte_test(rlc_am_stress_test rlc_stress_test --mode=AM --loglevel 1 --sdu_gen_delay 250)
add_lte_test(rlc_um_stress_test rlc_stress_test --mode=UM --loglevel 1)
add_lte_test(rlc_tm_stress_test rlc_stress_test --mode=TM --loglevel 1 --random_opp=false)

add_nr_test(rlc_um6_nr_stress_test rlc_stress_test --rat NR --mode=UM6 --loglevel 1)
add_nr_test(rlc_um12_nr_stress_test rlc_stress_test --rat NR --mode=UM12 --loglevel 1) 
add_nr_test(rlc_am12_nr_stress_test rlc_stress_test --rat NR --mode=AM12 --loglevel 1) 
add_nr_test(rlc_am12_nr_stress_test rlc_stress_test --rat NR --mode=AM18 --loglevel 1) 

add_executable(rlc_um_data_test rlc_um_data_test.cc)
target_link_libraries(rlc_um_data_test isrran_rlc isrran_phy isrran_common)
add_test(rlc_um_data_test rlc_um_data_test)

add_executable(rlc_um_test rlc_um_test.cc)
target_link_libraries(rlc_um_test isrran_rlc isrran_phy)
add_test(rlc_um_test rlc_um_test)

add_executable(rlc_common_test rlc_common_test.cc)
target_link_libraries(rlc_common_test isrran_rlc isrran_phy)
add_test(rlc_common_test rlc_common_test)

add_executable(rlc_um_nr_pdu_test rlc_um_nr_pdu_test.cc)
target_link_libraries(rlc_um_nr_pdu_test isrran_rlc isrran_mac isrran_phy)
add_nr_test(rlc_um_nr_pdu_test rlc_um_nr_pdu_test)

add_executable(rlc_um_nr_test rlc_um_nr_test.cc)
target_link_libraries(rlc_um_nr_test isrran_rlc isrran_phy isrran_mac isrran_common)
add_nr_test(rlc_um_nr_test rlc_um_nr_test)

########################################################################
# Option to run command after build (useful for remote builds)
########################################################################
if (NOT ${BUILD_CMD} STREQUAL "")
  message(STATUS "Added custom post-build command: ${BUILD_CMD}")
  add_custom_command(TARGET ip_test POST_BUILD COMMAND ${BUILD_CMD})
else(NOT ${BUILD_CMD} STREQUAL "")
  message(STATUS "No post-build command defined")
endif (NOT ${BUILD_CMD} STREQUAL "")
