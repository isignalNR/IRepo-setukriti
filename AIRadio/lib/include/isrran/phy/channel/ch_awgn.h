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

/**
 *  \file ch_awgn.h
 *  \brief Additive white Gaussian noise channel object
 *
 */

#include "isrran/config.h"
#include <stdint.h>

#ifndef ISRRAN_CH_AWGN_H
#define ISRRAN_CH_AWGN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief isrRAN channel AWGN implements an efficient Box-Muller Method accelerated with SIMD.
 */
typedef struct {
  float*   table_cos;
  float*   table_log;
  uint32_t rand_state;
  float    std_dev;
} isrran_channel_awgn_t;

/**
 * \brief function of the channel AWGN object
 *
 * @param q AWGN channel object
 * @param seed random generator seed
 */
ISRRAN_API int isrran_channel_awgn_init(isrran_channel_awgn_t* q, uint32_t seed);

/**
 * \brief the noise level N0 in decibels full scale (dBfs)
 *
 * @param q AWGN channel object
 * @param n0_dBfs noise level
 */
ISRRAN_API int isrran_channel_awgn_set_n0(isrran_channel_awgn_t* q, float n0_dBfs);

/**
 * \brief the complex AWGN channel
 *
 * @param q AWGN channel object
 * @param in complex input array
 * @param out complex output array
 * @param length number of samples
 */
ISRRAN_API void isrran_channel_awgn_run_c(isrran_channel_awgn_t* q, const cf_t* in, cf_t* out, uint32_t length);

/**
 * \brief the real AWGN channel
 *
 * @param q AWGN channel object
 * @param in real input array
 * @param out real output array
 * @param length number of samples
 */
ISRRAN_API void isrran_channel_awgn_run_f(isrran_channel_awgn_t* q, const float* in, float* out, uint32_t length);

/**
 * \brief AWGN channel generator data
 *
 * @param q AWGN channel object
 */
ISRRAN_API void isrran_channel_awgn_free(isrran_channel_awgn_t* q);

/**
 * \brief signal \p input with AWGN to obtain signal \p output (complex case).
 *
 * @param[in] input Input signal
 * @param[out] output Output signal
 * @param[in] variance Noise variance
 * @param[in] len Number of samples
 */
ISRRAN_API void isrran_ch_awgn_c(const cf_t* input, cf_t* output, float variance, uint32_t len);

/**
 * \brief Perturb signal \p input with AWGN to obtain signal \p output (real case).
 *
 * @param[in] input Input signal
 * @param[out] output Output signal
 * @param[in] variance Noise variance
 * @param[in] len Number of samples
 */
ISRRAN_API void isrran_ch_awgn_f(const float* input, float* output, float variance, uint32_t len);

ISRRAN_API float isrran_ch_awgn_get_variance(float ebno_db, float rate);

#ifdef __cplusplus
}
#endif

#endif // ISRRAN_CH_AWGN_H
