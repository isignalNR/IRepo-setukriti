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

add_executable(rrc_reconfig_test rrc_reconfig_test.cc)
target_link_libraries(rrc_reconfig_test isrue_upper isrran_phy isrran_asn1 rrc_asn1 isrlog)
add_test(rrc_reconfig_test rrc_reconfig_test)

add_executable(rrc_meas_test rrc_meas_test.cc)
target_link_libraries(rrc_meas_test isrue_rrc isrue_upper isrran_pdcp isrran_phy rrc_asn1 rrc_nr_asn1)
add_test(rrc_meas_test rrc_meas_test)

add_executable(rrc_phy_ctrl_test rrc_phy_ctrl_test.cc)
target_link_libraries(rrc_phy_ctrl_test isrran_common isrue_rrc ${ATOMIC_LIBS})
add_test(rrc_phy_ctrl_test rrc_phy_ctrl_test)

add_executable(rrc_cell_test rrc_cell_test.cc)
target_link_libraries(rrc_cell_test isrue_rrc isrue_upper isrran_pdcp isrran_phy rrc_asn1 rrc_nr_asn1)
add_test(rrc_cell_test rrc_cell_test)

add_executable(rrc_rlf_report_test rrc_rlf_report_test.cc)
target_link_libraries(rrc_rlf_report_test isrue_rrc isrue_upper isrran_pdcp isrran_phy rrc_asn1 rrc_nr_asn1)
add_test(rrc_rlf_report_test rrc_rlf_report_test)