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

#ifndef ISRRAN_SCHED_NR_WORKER_H
#define ISRRAN_SCHED_NR_WORKER_H

#include "sched_nr_bwp.h"
#include "sched_nr_cfg.h"
#include "sched_nr_grant_allocator.h"
#include "sched_nr_ue.h"
#include "isrran/adt/circular_array.h"
#include "isrran/adt/optional.h"
#include "isrran/adt/pool/cached_alloc.h"
#include "isrran/adt/span.h"
#include <condition_variable>
#include <mutex>

namespace isrenb {

struct mac_metrics_t;

namespace sched_nr_impl {

class cc_worker
{
public:
  explicit cc_worker(const cell_config_manager& params);

  void dl_rach_info(const sched_nr_interface::rar_info_t& rar_info);

  dl_sched_res_t* run_slot(slot_point pdcch_slot, ue_map_t& ue_db_);
  ul_sched_t*     get_ul_sched(slot_point sl);

  // const params
  const cell_config_manager& cfg;
  isrlog::basic_logger&      logger;

  // cc-specific resources
  isrran::bounded_vector<bwp_manager, SCHED_NR_MAX_BWP_PER_CELL> bwps;

private:
  void alloc_dl_ues(bwp_slot_allocator& bwp_alloc);
  void alloc_ul_ues(bwp_slot_allocator& bwp_alloc);
  void postprocess_decisions(bwp_slot_allocator& bwp_alloc);

  // {slot,cc} specific variables
  slot_ue_map_t slot_ues;

  slot_point last_tx_sl;
};

} // namespace sched_nr_impl
} // namespace isrenb

#endif // ISRRAN_SCHED_NR_WORKER_H
