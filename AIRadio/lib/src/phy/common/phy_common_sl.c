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

#include <string.h>

#include "isrran/phy/common/phy_common_sl.h"
#include "isrran/phy/common/sequence.h"
#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/vector.h"

int isrran_sl_group_hopping_f_gh(uint32_t f_gh[ISRRAN_NSLOTS_X_FRAME * 2], uint32_t N_x_id)
{
  isrran_sequence_t seq;
  bzero(&seq, sizeof(isrran_sequence_t));

  if (isrran_sequence_LTE_pr(&seq, 160 * 2, N_x_id / 30)) {
    return ISRRAN_ERROR;
  }

  for (uint32_t ns = 0; ns < ISRRAN_NSLOTS_X_FRAME * 2; ns++) {
    f_gh[ns] = 0;
    for (int i = 0; i < 8; i++) {
      f_gh[ns] += (((uint32_t)seq.c[8 * ns + i]) << i);
    }
  }

  isrran_sequence_free(&seq);
  return ISRRAN_SUCCESS;
}

bool isrran_slss_side_peak_pos_is_valid(uint32_t side_peak_pos,
                                        uint32_t main_peak_pos,
                                        uint32_t side_peak_delta_a,
                                        uint32_t side_peak_delta_b)
{
  if ((side_peak_pos >= (main_peak_pos - side_peak_delta_b)) &&
      (side_peak_pos <= (main_peak_pos - side_peak_delta_a))) {
    return true;
  } else if ((side_peak_pos >= (main_peak_pos + side_peak_delta_a)) &&
             (side_peak_pos <= (main_peak_pos + side_peak_delta_b))) {
    return true;
  }
  return false;
}

bool isrran_slss_side_peak_value_is_valid(float side_peak_value, float threshold_low, float threshold_high)
{
  if ((side_peak_value >= threshold_low) && (side_peak_value <= threshold_high)) {
    return true;
  }
  return false;
}

int isrran_sl_tm_to_cell_sl_tm_t(isrran_cell_sl_t* q, uint32_t tm)
{
  switch (tm) {
    case 1:
      q->tm = ISRRAN_SIDELINK_TM1;
      break;
    case 2:
      q->tm = ISRRAN_SIDELINK_TM2;
      break;
    case 3:
      q->tm = ISRRAN_SIDELINK_TM3;
      break;
    case 4:
      q->tm = ISRRAN_SIDELINK_TM4;
      break;
    default:
      return ISRRAN_ERROR;
  }
  return ISRRAN_SUCCESS;
}

/* Returns the number of OFDM symbols depending on the Sidelink transmission mode and the CP
 *
 * @param tm Sidelink transmission mode
 * @cp Cyclic prefix
 * @return Number of symbols (0 if invalid params where given)
 */
uint32_t isrran_sl_get_num_symbols(isrran_sl_tm_t tm, isrran_cp_t cp)
{
  if (tm == ISRRAN_SIDELINK_TM1 || tm == ISRRAN_SIDELINK_TM2) {
    if (cp == ISRRAN_CP_NORM) {
      return ISRRAN_CP_NORM_SF_NSYMB;
    } else {
      return ISRRAN_CP_EXT_SF_NSYMB;
    }
  } else if (tm == ISRRAN_SIDELINK_TM3 || tm == ISRRAN_SIDELINK_TM4) {
    if (cp == ISRRAN_CP_NORM) {
      return ISRRAN_CP_NORM_SF_NSYMB;
    } else {
      ERROR("Invalid CP");
    }
  } else {
    ERROR("Invalid TM");
  }
  return 0; // Calling function may use return value for loop termination
}

// Look-up tables for Sidelink channel symbols
static const isrran_sl_symbol_t isrran_psbch_symbol_map_tm12[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_psbch_symbol_map_tm12_ext[ISRRAN_CP_EXT_SF_NSYMB] = {
    ISRRAN_SIDELINK_SYNC_SYMBOL,
    ISRRAN_SIDELINK_SYNC_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_SYNC_SYMBOL,
    ISRRAN_SIDELINK_SYNC_SYMBOL,
    ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_psbch_symbol_map_tm34[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_SYNC_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

bool isrran_psbch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp)
{
  if (tm == ISRRAN_SIDELINK_TM1 || tm == ISRRAN_SIDELINK_TM2) {
    if (cp == ISRRAN_CP_EXT && i < ISRRAN_CP_EXT_SF_NSYMB) {
      return isrran_psbch_symbol_map_tm12_ext[i] == type;
    } else if (i < ISRRAN_CP_NORM_SF_NSYMB) {
      return isrran_psbch_symbol_map_tm12[i] == type;
    }
  } else if ((tm == ISRRAN_SIDELINK_TM3 || tm == ISRRAN_SIDELINK_TM4) && i < ISRRAN_CP_NORM_SF_NSYMB) {
    return isrran_psbch_symbol_map_tm34[i] == type;
  }
  return false;
}

static const isrran_sl_symbol_t isrran_pscch_symbol_map_tm12[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_pscch_symbol_map_tm12_ext[ISRRAN_CP_EXT_SF_NSYMB] = {
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_pscch_symbol_map_tm34[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

bool isrran_pscch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp)
{
  if (tm == ISRRAN_SIDELINK_TM1 || tm == ISRRAN_SIDELINK_TM2) {
    if (cp == ISRRAN_CP_EXT && i < ISRRAN_CP_EXT_SF_NSYMB) {
      return isrran_pscch_symbol_map_tm12_ext[i] == type;
    } else if (i < ISRRAN_CP_NORM_SF_NSYMB) {
      return isrran_pscch_symbol_map_tm12[i] == type;
    }
  } else if ((tm == ISRRAN_SIDELINK_TM3 || tm == ISRRAN_SIDELINK_TM4) && i < ISRRAN_CP_NORM_SF_NSYMB) {
    return isrran_pscch_symbol_map_tm34[i] == type;
  }
  return false;
}

static const isrran_sl_symbol_t isrran_pssch_symbol_map_tm12[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_pssch_symbol_map_tm12_ext[ISRRAN_CP_EXT_SF_NSYMB] = {
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DMRS_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_DATA_SYMBOL,
    ISRRAN_SIDELINK_GUARD_SYMBOL};

static const isrran_sl_symbol_t isrran_pssch_symbol_map_tm34[ISRRAN_CP_NORM_SF_NSYMB] = {ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DMRS_SYMBOL,
                                                                                         ISRRAN_SIDELINK_DATA_SYMBOL,
                                                                                         ISRRAN_SIDELINK_GUARD_SYMBOL};

bool isrran_pssch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp)
{
  if (tm == ISRRAN_SIDELINK_TM1 || tm == ISRRAN_SIDELINK_TM2) {
    if (cp == ISRRAN_CP_EXT && i < ISRRAN_CP_EXT_SF_NSYMB) {
      return isrran_pssch_symbol_map_tm12_ext[i] == type;
    } else if (i < ISRRAN_CP_NORM_SF_NSYMB) {
      return isrran_pssch_symbol_map_tm12[i] == type;
    }
  } else if ((tm == ISRRAN_SIDELINK_TM3 || tm == ISRRAN_SIDELINK_TM4) && i < ISRRAN_CP_NORM_SF_NSYMB) {
    return isrran_pssch_symbol_map_tm34[i] == type;
  }
  return false;
}

uint32_t isrran_sci_format0_sizeof(uint32_t nof_prb)
{
  // 3GPP TS 36.212 5.4.3.1
  uint32_t n = 0;

  // Frequency hopping flag – 1 bit
  n += 1;

  // Resource block assignment and hopping resource allocation
  n += (uint32_t)ceil(log((nof_prb * (nof_prb + 1)) / 2.0) / log(2));

  // Time resource pattern – 7 bits
  n += 7;

  // Modulation and coding scheme – 5 bit
  n += 5;

  // Timing advance indication – 11 bits
  n += 11;

  // Group destination ID – 8 bits
  n += 8;

  return n;
}

// Example pool configurations based on 3GPP TS 36.101 Version 15.6 Sections A.7 and A.9
int isrran_sl_comm_resource_pool_get_default_config(isrran_sl_comm_resource_pool_t* q, isrran_cell_sl_t cell)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;
  if (q != NULL) {
    q->period_length = 40;
    if (cell.tm == ISRRAN_SIDELINK_TM3 || cell.tm == ISRRAN_SIDELINK_TM4) {
      q->period_length = 160;
    }

    // Use full Bandwidth
    q->prb_num   = (uint32_t)ceil(cell.nof_prb / 2.0);
    q->prb_start = 0;
    q->prb_end   = cell.nof_prb - 1;

    // 0110000000000000000000000000000000000000
    bzero(q->pscch_sf_bitmap, ISRRAN_SL_MAX_PERIOD_LENGTH);
    memset(&q->pscch_sf_bitmap[1], 1, 2);

    // 0001111111111111111111111111111111111111
    bzero(q->pssch_sf_bitmap, ISRRAN_SL_MAX_PERIOD_LENGTH);
    memset(&q->pssch_sf_bitmap[3], 1, 37);

    q->size_sub_channel      = 10;
    q->num_sub_channel       = 5;
    q->start_prb_sub_channel = 0;
    q->adjacency_pscch_pssch = true;

    q->sf_bitmap_tm34_len = 10;
    bzero(q->sf_bitmap_tm34, ISRRAN_SL_MAX_PERIOD_LENGTH);
    memset(q->sf_bitmap_tm34, 1, q->sf_bitmap_tm34_len);

    if (cell.tm == ISRRAN_SIDELINK_TM4) {
      switch (cell.nof_prb) {
        case 6:
          q->size_sub_channel = 6;
          q->num_sub_channel  = 1;
          break;
        case 15:
        case 25:
        case 75:
          q->size_sub_channel = 5;
          q->num_sub_channel  = cell.nof_prb / q->size_sub_channel;
          break;
        case 50:
        case 100:
          q->size_sub_channel = 10;
          q->num_sub_channel  = cell.nof_prb / q->size_sub_channel;
          break;
        default:
          ERROR("Invalid nof_prb");
          return ISRRAN_ERROR;
      }
    }

    ret = ISRRAN_SUCCESS;
  }
  return ret;
}
