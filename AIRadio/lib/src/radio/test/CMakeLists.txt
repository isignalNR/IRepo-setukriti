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

if(RF_FOUND)
  add_executable(benchmark_radio benchmark_radio.cc)
  target_link_libraries(benchmark_radio isrran_common isrran_phy isrran_radio)
  if(ISRGUI_FOUND)
    include_directories(${ISRGUI_INCLUDE_DIRS})
    target_link_libraries(benchmark_radio ${ISRGUI_LIBRARIES})
  endif(ISRGUI_FOUND)
  if (ZEROMQ_FOUND)
    add_test(benchmark_radio_multi_rf benchmark_radio -d zmq -a
            tx_port=tcp://*:2000,rx_port=tcp://localhost:2000\;tx_port=tcp://*:2001,rx_port=tcp://localhost:2001\;tx_port=tcp://*:2002,rx_port=tcp://localhost:2002\;tx_port=tcp://*:2003,rx_port=tcp://localhost:2003\;
            -p 4)
  endif (ZEROMQ_FOUND)

  add_executable(test_radio_rt_gain test_radio_rt_gain.cc)
  target_link_libraries(test_radio_rt_gain
          isrran_common
          isrran_phy
          isrran_radio
          ${CMAKE_THREAD_LIBS_INIT}
          ${Boost_LIBRARIES})
  if (ZEROMQ_FOUND)
    add_test(test_radio_rt_gain_zmq test_radio_rt_gain --srate=3.84e6 --dev_name=zmq --dev_args=tx_port=ipc:///tmp/test_radio_rt_gain_zmq,rx_port=ipc:///tmp/test_radio_rt_gain_zmq,base_srate=3.84e6)
  endif (ZEROMQ_FOUND)

endif(RF_FOUND)


