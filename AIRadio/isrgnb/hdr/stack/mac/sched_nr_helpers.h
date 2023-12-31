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

#ifndef ISRRAN_SCHED_NR_HELPERS_H
#define ISRRAN_SCHED_NR_HELPERS_H

#include "sched_nr_cfg.h"
#include "sched_nr_ue.h"
#include "isrran/adt/optional_array.h"

namespace isrenb {
namespace sched_nr_impl {

class slot_ue;
class ul_harq_proc;
struct bwp_res_grid;

/// Helper function to verify if RNTI type can be placed in specified search space
/// Based on 38.213, Section 10.1
inline bool is_rnti_type_valid_in_search_space(isrran_rnti_type_t rnti_type, isrran_search_space_type_t ss_type)
{
  switch (ss_type) {
    case isrran_search_space_type_common_0:  // fall-through
    case isrran_search_space_type_common_0A: // Other SIBs
      return rnti_type == isrran_rnti_type_si;
    case isrran_search_space_type_common_1:
      return rnti_type == isrran_rnti_type_ra or rnti_type == isrran_rnti_type_tc or
             /* in case of Pcell -> */ rnti_type == isrran_rnti_type_c;
    case isrran_search_space_type_common_2:
      return rnti_type == isrran_rnti_type_p;
    case isrran_search_space_type_common_3:
      return rnti_type == isrran_rnti_type_c; // TODO: Fix
    case isrran_search_space_type_ue:
      return rnti_type == isrran_rnti_type_c or rnti_type == isrran_rnti_type_cs or
             rnti_type == isrran_rnti_type_sp_csi;
    default:
      break;
  }
  return false;
}

/// Log UE state for slot being scheduled
void log_sched_slot_ues(isrlog::basic_logger& logger,
                        slot_point            pdcch_slot,
                        uint32_t              cc,
                        const slot_ue_map_t&  slot_ues);

/// Log Scheduling Result for a given BWP and slot
void log_sched_bwp_result(isrlog::basic_logger& logger,
                          slot_point            pdcch_slot,
                          const bwp_res_grid&   res_grid,
                          const slot_ue_map_t&  slot_ues);

} // namespace sched_nr_impl
} // namespace isrenb

#endif // ISRRAN_SCHED_NR_HELPERS_H
