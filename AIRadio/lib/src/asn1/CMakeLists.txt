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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-unused-variable -Wno-return-type -Wno-sign-compare -Wno-reorder -Wno-parentheses -fno-exceptions")
add_library(isrran_asn1 STATIC
  liblte_common.cc
  liblte_mme.cc
  gtpc.cc
)

# ASN1 utils
add_library(asn1_utils STATIC asn1_utils.cc)
target_link_libraries(asn1_utils isrran_common)
install(TARGETS asn1_utils DESTINATION ${LIBRARY_DIR} OPTIONAL)

# RRC ASN1 lib
add_library(rrc_asn1 STATIC
    rrc.cc
    rrc/bcch_msg.cc
    rrc/common.cc
    rrc/common_ext.cc
    rrc/dl_ccch_msg.cc
    rrc/dl_dcch_msg.cc
    rrc/ho_cmd.cc
    rrc/meascfg.cc
    rrc/paging.cc
    rrc/phy_ded.cc
    rrc/rr_common.cc
    rrc/rr_ded.cc
    rrc/security.cc
    rrc/si.cc
    rrc/uecap.cc
    rrc/ul_ccch_msg.cc
    rrc/ul_dcch_msg.cc
    rrc_nbiot.cc
    rrc_utils.cc)
# Compile RRC ASN1 optimized for size
target_compile_options(rrc_asn1 PRIVATE "-Os")
target_link_libraries(rrc_asn1 asn1_utils isrran_common)
install(TARGETS rrc_asn1 DESTINATION ${LIBRARY_DIR} OPTIONAL)

# S1AP ASN1 lib
add_library(s1ap_asn1 STATIC
            s1ap.cc s1ap_utils.cc)
target_compile_options(s1ap_asn1 PRIVATE "-Os")
target_link_libraries(s1ap_asn1 asn1_utils isrran_common)
install(TARGETS s1ap_asn1 DESTINATION ${LIBRARY_DIR} OPTIONAL)

# RRC NR ASN1
add_library(rrc_nr_asn1 STATIC rrc_nr.cc rrc_nr_utils.cc)
target_compile_options(rrc_nr_asn1 PRIVATE "-Os")
target_link_libraries(rrc_nr_asn1 asn1_utils isrran_common)
install(TARGETS rrc_nr_asn1 DESTINATION ${LIBRARY_DIR} OPTIONAL)
# NGAP ASN1
add_library(ngap_nr_asn1 STATIC ngap.cc)
target_compile_options(ngap_nr_asn1 PRIVATE "-Os")
target_link_libraries(ngap_nr_asn1 asn1_utils isrran_common)
install(TARGETS ngap_nr_asn1 DESTINATION ${LIBRARY_DIR} OPTIONAL)
# NAS 5G
add_library(nas_5g_msg STATIC nas_5g_msg.cc nas_5g_ies.cc nas_5g_utils.cc)
target_compile_options(nas_5g_msg PRIVATE "-Os")
target_link_libraries(nas_5g_msg asn1_utils isrran_common)
install(TARGETS nas_5g_msg DESTINATION ${LIBRARY_DIR} OPTIONAL)


