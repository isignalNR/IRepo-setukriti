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

add_executable(thread_pool_test thread_pool_test.cc)
target_link_libraries(thread_pool_test
        isrran_common)
add_test(thread_pool_test thread_pool_test)

add_executable(thread_test thread_test.cc)
target_link_libraries(thread_test
        isrran_common)
add_test(thread_test thread_test)

add_executable(band_helper_test band_helper_test.cc)
target_link_libraries(band_helper_test isrran_common)
add_test(band_helper_test band_helper_test)