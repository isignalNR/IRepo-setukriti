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

#ifndef ISRRAN_SCHED_RESULT_H
#define ISRRAN_SCHED_RESULT_H

#include "../sched_lte_common.h"
#include "isrenb/hdr/stack/mac/sched_phy_ch/sched_phy_resource.h"

namespace isrenb {

/// Result of a Subframe sched computation
struct cc_sched_result {
  bool      generated = false;
  tti_point tti_rx{};

  /// Accumulation of all DL RBG allocations
  rbgmask_t dl_mask = {};

  /// Accumulation of all UL PRB allocations
  prbmask_t ul_mask = {};

  /// Accumulation of all CCE allocations
  pdcch_mask_t pdcch_mask = {};

  /// Individual allocations information
  sched_interface::dl_sched_res_t dl_sched_result = {};
  sched_interface::ul_sched_res_t ul_sched_result = {};
};

} // namespace isrenb

#endif // ISRRAN_SCHED_RESULT_H
