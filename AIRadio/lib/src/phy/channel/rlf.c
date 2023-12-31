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

#include <isrran/phy/channel/rlf.h>
#include <isrran/phy/utils/vector.h>

void isrran_channel_rlf_init(isrran_channel_rlf_t* q, uint32_t t_on_ms, uint32_t t_off_ms)
{
  q->t_on_ms  = t_on_ms;
  q->t_off_ms = t_off_ms;
}

void isrran_channel_rlf_execute(isrran_channel_rlf_t*     q,
                                const cf_t*               in,
                                cf_t*                     out,
                                uint32_t                  nsamples,
                                const isrran_timestamp_t* ts)
{
  // Caulculate full period in MS
  uint64_t period_ms = q->t_on_ms + q->t_off_ms;

  // Convert seconds to ms and reduce it into 32 bit
  uint32_t full_secs_ms = (uint32_t)((ts->full_secs * 1000UL) % period_ms);

  // Convert Fractional seconds into ms and convert it to integer
  uint32_t frac_secs_ms = (uint32_t)round(ts->frac_secs * 1000);

  // Add full seconds and fractional performing period module
  uint32_t time_ms = (full_secs_ms + frac_secs_ms) % period_ms;

  // Decide whether enables or disables channel
  if (time_ms < q->t_on_ms) {
    isrran_vec_sc_prod_cfc(in, 1.0f, out, nsamples);
  } else {
    isrran_vec_sc_prod_cfc(in, 0.0f, out, nsamples);
  }
}

void isrran_channel_rlf_free(isrran_channel_rlf_t* q)
{
  // Do nothing
}
