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

#ifndef ISRUE_SFN_SYNC_H
#define ISRUE_SFN_SYNC_H

#include "isrran/radio/radio.h"
#include "isrran/isrlog/isrlog.h"
#include "isrran/isrran.h"

namespace isrue {

struct phy_args_t;

// Class to synchronize system frame number
class sfn_sync
{
public:
  typedef enum { IDLE, SFN_FOUND, SFX0_FOUND, SFN_NOFOUND, ERROR } ret_code;
  explicit sfn_sync(isrlog::basic_logger& logger) : logger(logger) {}
  ~sfn_sync();
  void     init(isrran_ue_sync_t*    ue_sync,
                const phy_args_t*    phy_args_,
                isrran::rf_buffer_t& buffer,
                uint32_t             buffer_max_samples_,
                uint32_t             nof_subframes = SFN_SYNC_NOF_SUBFRAMES);
  void     reset();
  bool     set_cell(isrran_cell_t cell);
  ret_code run_subframe(isrran_cell_t*                               cell,
                        uint32_t*                                    tti_cnt,
                        std::array<uint8_t, ISRRAN_BCH_PAYLOAD_LEN>& bch_payload,
                        bool                                         sfidx_only = false);
  ret_code decode_mib(isrran_cell_t*                               cell,
                      uint32_t*                                    tti_cnt,
                      isrran::rf_buffer_t*                         ext_buffer,
                      std::array<uint8_t, ISRRAN_BCH_PAYLOAD_LEN>& bch_payload,
                      bool                                         sfidx_only = false);

private:
  const static int SFN_SYNC_NOF_SUBFRAMES = 100;

  const phy_args_t*     phy_args = nullptr;
  uint32_t              cnt      = 0;
  uint32_t              timeout  = 0;
  isrlog::basic_logger& logger;
  isrran_ue_sync_t*     ue_sync            = nullptr;
  isrran::rf_buffer_t   mib_buffer         = {};
  uint32_t              buffer_max_samples = 0;
  isrran_ue_mib_t       ue_mib             = {};
};

}; // namespace isrue

#endif // ISRUE_SFN_SYNC_H
