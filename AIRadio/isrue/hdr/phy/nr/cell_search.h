/**
 * Copyright 2013-2022 iSignal Research Labs Pvt Ltd.
 *
 * This file is part of isrRAN.
 *
 * isrRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * isrRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef ISRUE_CELL_SEARCH_H
#define ISRUE_CELL_SEARCH_H

#include "isrran/interfaces/radio_interfaces.h"
#include "isrran/interfaces/ue_nr_interfaces.h"
#include "isrran/isrran.h"

namespace isrue {
namespace nr {
class cell_search
{
public:
  struct args_t {
    double                      max_srate_hz;
    isrran_subcarrier_spacing_t ssb_min_scs = isrran_subcarrier_spacing_15kHz;
  };

  struct cfg_t {
    double                      srate_hz;
    double                      center_freq_hz;
    double                      ssb_freq_hz;
    isrran_subcarrier_spacing_t ssb_scs;
    isrran_ssb_pattern_t        ssb_pattern;
    isrran_duplex_mode_t        duplex_mode;
  };

  struct ret_t {
    enum { CELL_FOUND = 1, CELL_NOT_FOUND = 0, ERROR = -1 } result;
    isrran_ssb_search_res_t ssb_res;
  };

  cell_search(isrlog::basic_logger& logger);
  ~cell_search();

  bool init(const args_t& args);

  bool  start(const cfg_t& cfg);
  ret_t run_slot(const cf_t* buffer, uint32_t slot_sz);

private:
  isrlog::basic_logger&        logger;
  isrran_ssb_t                 ssb    = {};
};
} // namespace nr
} // namespace isrue

#endif // ISRUE_CELL_SEARCH_H
