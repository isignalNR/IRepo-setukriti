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

#ifndef ISRRAN_RF_UTILS_H
#define ISRRAN_RF_UTILS_H

#include "isrran/phy/rf/rf.h"
#include "isrran/isrran.h"

typedef struct ISRRAN_API {
  uint32_t max_frames_pbch;      // timeout in number of 5ms frames for MIB decoding
  uint32_t max_frames_pss;       // timeout in number of 5ms frames for synchronization
  uint32_t nof_valid_pss_frames; // number of required synchronized frames
  float    init_agc;             // 0 or negative to disable AGC
  bool     force_tdd;
} cell_search_cfg_t;

ISRRAN_API int rf_rssi_scan(isrran_rf_t* rf, float* freqs, float* rssi, int nof_bands, double fs, int nsamp);

ISRRAN_API int
rf_mib_decoder(isrran_rf_t* rf, uint32_t nof_rf_channels, cell_search_cfg_t* config, isrran_cell_t* cell, float* cfo);

ISRRAN_API int rf_cell_search(isrran_rf_t*       rf,
                              uint32_t           nof_rf_channels,
                              cell_search_cfg_t* config,
                              int                force_N_id_2,
                              isrran_cell_t*     cell,
                              float*             cfo);

ISRRAN_API int rf_search_and_decode_mib(isrran_rf_t*       rf,
                                        uint32_t           nof_rf_channels,
                                        cell_search_cfg_t* config,
                                        int                force_N_id_2,
                                        isrran_cell_t*     cell,
                                        float*             cfo);

ISRRAN_API int rf_cell_search_nbiot(isrran_rf_t* rf, cell_search_cfg_t* config, isrran_nbiot_cell_t* cell, float* cfo);

#endif // ISRRAN_RF_UTILS_H
