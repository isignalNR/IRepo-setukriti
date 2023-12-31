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

#######################################################################
# ADT TESTS
#######################################################################

add_executable(scope_exit_test scope_exit_test.cc)
target_link_libraries(scope_exit_test isrran_common)
add_test(scope_exit_test scope_exit_test)

add_executable(expected_test expected_test.cc)
target_link_libraries(expected_test isrran_common)
add_test(expected_test expected_test)

add_executable(bounded_bitset_test bounded_bitset_test.cc)
target_link_libraries(bounded_bitset_test isrran_common)
add_test(bounded_bitset_test bounded_bitset_test)

add_executable(span_test span_test.cc)
target_link_libraries(span_test isrran_common)
add_test(span_test span_test)

add_executable(interval_test interval_test.cc)
target_link_libraries(interval_test isrran_common)
add_test(interval_test interval_test)

add_executable(observer_test observer_test.cc)
target_link_libraries(observer_test isrran_common)
add_test(observer_test observer_test)

add_executable(bounded_vector_test bounded_vector_test.cc)
target_link_libraries(bounded_vector_test isrran_common)
add_test(bounded_vector_test bounded_vector_test)

add_executable(mem_pool_test mem_pool_test.cc)
target_link_libraries(mem_pool_test isrran_common)
add_test(mem_pool_test mem_pool_test)

add_executable(circular_buffer_test circular_buffer_test.cc)
target_link_libraries(circular_buffer_test isrran_common)
add_test(circular_buffer_test circular_buffer_test)

add_executable(circular_map_test circular_map_test.cc)
target_link_libraries(circular_map_test isrran_common)
add_test(circular_map_test circular_map_test)

add_executable(fsm_test fsm_test.cc)
target_link_libraries(fsm_test isrran_common)
add_test(fsm_test fsm_test)

add_executable(optional_test optional_test.cc)
target_link_libraries(optional_test isrran_common)
add_test(optional_test optional_test)

add_executable(cached_alloc_test cached_alloc_test.cc)
target_link_libraries(cached_alloc_test isrran_common)
add_test(cached_alloc_test cached_alloc_test)

add_executable(optional_array_test optional_array_test.cc)
target_link_libraries(optional_array_test isrran_common)
add_test(optional_array_test optional_array_test)
