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

#include <assert.h>
#include <complex.h>
#include <stdlib.h>
#include <strings.h>

#include "isrran/phy/common/phy_common.h"
#include "isrran/phy/sync/nsss.h"
#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/vector.h"

#define PRINT_ERR(err) fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, err)

#define DUMP_SIGNALS 0
#define DO_FREQ_SHIFT 1
#define ISRRAN_NSSS_RETURN_PSR 0

int isrran_nsss_synch_init(isrran_nsss_synch_t* q, uint32_t input_size, uint32_t fft_size)
{
  if (q != NULL && fft_size <= 2048) {
    int ret = ISRRAN_ERROR;
    bzero(q, sizeof(isrran_nsss_synch_t));

    q->fft_size = q->max_fft_size = fft_size;

    q->input_size          = input_size;
    q->corr_peak_threshold = 2.0;

    uint32_t buffer_size = ISRRAN_NSSS_CORR_FILTER_LEN + q->input_size + 1;
    DEBUG("NSSS buffer size is %d samples.", buffer_size);
    q->tmp_input = isrran_vec_cf_malloc(buffer_size);
    if (!q->tmp_input) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    isrran_vec_cf_zero(q->tmp_input, buffer_size);

    q->conv_output = isrran_vec_cf_malloc(buffer_size);
    if (!q->conv_output) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    isrran_vec_cf_zero(q->conv_output, buffer_size);

    q->conv_output_abs = isrran_vec_f_malloc(buffer_size);
    if (!q->conv_output_abs) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    isrran_vec_f_zero(q->conv_output_abs, buffer_size);

    for (int i = 0; i < ISRRAN_NUM_PCI; i++) {
      q->nsss_signal_time[i] = isrran_vec_cf_malloc(buffer_size);
      if (!q->nsss_signal_time[i]) {
        fprintf(stderr, "Error allocating memory\n");
        goto clean_and_exit;
      }
      isrran_vec_cf_zero(q->nsss_signal_time[i], buffer_size);
    }

    // generate NSSS sequences
    if (isrran_nsss_corr_init(q)) {
      fprintf(stderr, "Error initiating NSSS detector for fft_size=%d\n", fft_size);
      goto clean_and_exit;
    }

    if (isrran_conv_fft_cc_init(&q->conv_fft, q->input_size, ISRRAN_NSSS_CORR_FILTER_LEN)) {
      fprintf(stderr, "Error initiating convolution FFT\n");
      goto clean_and_exit;
    }

    ret = ISRRAN_SUCCESS;

  clean_and_exit:
    if (ret == ISRRAN_ERROR) {
      isrran_nsss_synch_free(q);
    }
    return ret;
  }
  return ISRRAN_ERROR_INVALID_INPUTS;
}

void isrran_nsss_synch_free(isrran_nsss_synch_t* q)
{
  if (q) {
    for (int i = 0; i < ISRRAN_NUM_PCI; i++) {
      if (q->nsss_signal_time[i]) {
        free(q->nsss_signal_time[i]);
      }
    }
    isrran_conv_fft_cc_free(&q->conv_fft);
    if (q->tmp_input) {
      free(q->tmp_input);
    }
    if (q->conv_output) {
      free(q->conv_output);
    }
    if (q->conv_output_abs) {
      free(q->conv_output_abs);
    }
  }
}

int isrran_nsss_synch_resize(isrran_nsss_synch_t* q, uint32_t fft_size)
{
  if (q != NULL && fft_size <= 2048) {
    if (fft_size > q->max_fft_size) {
      PRINT_ERR("fft_size must be lower than initialized\n");
      return ISRRAN_ERROR;
    }

    q->fft_size = fft_size;

    if (isrran_nsss_corr_init(q) != ISRRAN_SUCCESS) {
      PRINT_ERR("Couldn't initialize NSSS sequence\n");
      return ISRRAN_ERROR;
    }

    return ISRRAN_SUCCESS;
  }
  return ISRRAN_ERROR_INVALID_INPUTS;
}

int isrran_nsss_corr_init(isrran_nsss_synch_t* q)
{
  isrran_dft_plan_t plan;
  float complex     nsss_signal_pad[q->fft_size];

  // construct dft plan
  if (isrran_dft_plan(&plan, q->fft_size, ISRRAN_DFT_BACKWARD, ISRRAN_DFT_COMPLEX)) {
    return ISRRAN_ERROR;
  }
  isrran_dft_plan_set_mirror(&plan, true);
  isrran_dft_plan_set_dc(&plan, false);
  isrran_dft_plan_set_norm(&plan, true);

#if DO_FREQ_SHIFT
  // shift entire signal in frequency domain by half a subcarrier
  cf_t  shift_buffer[ISRRAN_SF_LEN(q->fft_size)];
  cf_t* ptr = shift_buffer;
  for (uint32_t n = 0; n < 2; n++) {
    for (uint32_t i = 0; i < 7; i++) {
      uint32_t cplen = ISRRAN_CP_LEN_NORM(i, q->fft_size);
      for (uint32_t t = 0; t < q->fft_size + cplen; t++) {
        ptr[t] = cexpf(I * 2 * M_PI * ((float)t - (float)cplen) * -ISRRAN_NBIOT_FREQ_SHIFT_FACTOR / q->fft_size);
      }
      ptr += q->fft_size + cplen;
    }
  }
#endif

  // generate correlation sequences
  DEBUG("Generating NSSS sequences");
  for (int i = 0; i < ISRRAN_NUM_PCI; i++) {
    float complex nsss_signal[ISRRAN_NSSS_TOT_LEN] = {};
    isrran_nsss_generate(nsss_signal, i);

    // one symbol at a time
    cf_t* output     = q->nsss_signal_time[i];
    int   output_len = 0;
    for (int i = 0; i < ISRRAN_NSSS_NSYMB; i++) {
      // zero buffer, copy NSSS symbol to appr. pos and transform to time-domain
      isrran_vec_cf_zero(nsss_signal_pad, q->fft_size);

      // 5th NSSS symbol has CP length of 10 symbols
      int cp_len =
          (i != 4) ? ISRRAN_CP_LEN_NORM(1, ISRRAN_NBIOT_FFT_SIZE) : ISRRAN_CP_LEN_NORM(0, ISRRAN_NBIOT_FFT_SIZE);
      int k = (q->fft_size - ISRRAN_NRE) / 2; // place seq in the centre
      k     = 57;

      // use generated sequence for theta_f = 0
      int theta_f = 0;
      memcpy(&nsss_signal_pad[k],
             &nsss_signal[(theta_f * ISRRAN_NSSS_LEN) + i * ISRRAN_NSSS_NSC],
             ISRRAN_NSSS_NSC * sizeof(cf_t));
      isrran_dft_run_c(&plan, nsss_signal_pad, &output[cp_len]);

      // add CP
      memcpy(output, &output[q->fft_size], cp_len * sizeof(cf_t));

      // prepare next iteration
      output += q->fft_size + cp_len;
      output_len += q->fft_size + cp_len;
    }
    assert(output_len == ISRRAN_NSSS_CORR_FILTER_LEN);

#if DO_FREQ_SHIFT
    isrran_vec_prod_ccc(q->nsss_signal_time[i],
                        &shift_buffer[ISRRAN_NSSS_CORR_OFFSET],
                        q->nsss_signal_time[i],
                        ISRRAN_NSSS_CORR_FILTER_LEN);
    // isrran_vec_sc_prod_cfc(npss_signal_time, 1.0/3, npss_signal_time, output_len);
#endif

#if DUMP_SIGNALS
#define MAX_FNAME_LEN 40
    char fname[MAX_FNAME_LEN];
    snprintf(fname, MAX_FNAME_LEN, "nsss_corr_seq_time_id%d.bin", i);
    isrran_vec_save_file(fname, q->nsss_signal_time[i], ISRRAN_NSSS_CORR_FILTER_LEN * sizeof(cf_t));

    snprintf(fname, MAX_FNAME_LEN, "nsss_corr_seq_freq_id%d.bin", i);
    isrran_vec_save_file(fname, nsss_signal, ISRRAN_NSSS_TOT_LEN * sizeof(cf_t));
#endif
  }

  isrran_dft_plan_free(&plan);
  return ISRRAN_SUCCESS;
}

int isrran_nsss_sync_find(isrran_nsss_synch_t* q,
                          cf_t*                input,
                          float*               corr_peak_value,
                          uint32_t*            cell_id,
                          uint32_t*            sfn_partial)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && input != NULL && corr_peak_value != NULL && cell_id != NULL && sfn_partial != NULL) {
    float peak_value;
    ret = ISRRAN_ERROR;

    // save input
    memcpy(q->tmp_input, input, q->input_size * sizeof(cf_t));

    if (*cell_id == ISRRAN_CELL_ID_UNKNOWN) {
      DEBUG("N_id_ncell is not set. Perform exhaustive search on input.");

      // brute-force: correlate with all possible sequences until cell is found
      for (int i = 0; i < ISRRAN_NUM_PCI; i++) {
        isrran_nsss_sync_find_pci(q, q->tmp_input, i);
      }

      // find maximum of all correlation maxima
      uint32_t max_id = isrran_vec_max_fi(q->peak_values, ISRRAN_NUM_PCI);
      if (q->peak_values[max_id] > q->corr_peak_threshold) {
        // cell found, set return values
        *cell_id = max_id;
        ret      = ISRRAN_SUCCESS;
      }
      peak_value = q->peak_values[max_id];

    } else {
      DEBUG("Current N_id_ncell is %d.", *cell_id);

      // run correlation only for given id
      isrran_nsss_sync_find_pci(q, q->tmp_input, *cell_id);

      if (q->peak_values[*cell_id] > q->corr_peak_threshold) {
        ret = ISRRAN_SUCCESS;
      }
      peak_value = q->peak_values[*cell_id];
    }

    // set remaining return values
    if (sfn_partial) {
      *sfn_partial = 0; // we only search for the first of the four possible shifts
    }
    if (corr_peak_value) {
      *corr_peak_value = peak_value;
    }
  }
  return ret;
}

// Correlates input signal with the NSSS sequence for a given n_id_ncell
void isrran_nsss_sync_find_pci(isrran_nsss_synch_t* q, cf_t* input, uint32_t cell_id)
{
  // correlate input with NSSS sequences
  uint32_t conv_output_len = isrran_corr_fft_cc_run(&q->conv_fft, input, q->nsss_signal_time[cell_id], q->conv_output);
  isrran_vec_abs_cf(q->conv_output, q->conv_output_abs, conv_output_len - 1);

  // Find maximum of the absolute value of the correlation
  uint32_t corr_peak_pos = isrran_vec_max_fi(q->conv_output_abs, conv_output_len - 1);

#if DUMP_SIGNALS
  printf("Dumping debug signals for cell-id %d.\n", i);
  isrran_vec_save_file("nsss_find_input.bin", input, q->input_size * sizeof(cf_t));
  isrran_vec_save_file("nsss_corr_seq_time.bin", q->nsss_signal_time[i], ISRRAN_NSSS_CORR_FILTER_LEN * sizeof(cf_t));
  isrran_vec_save_file("nsss_find_conv_output_abs.bin", q->conv_output_abs, conv_output_len * sizeof(float));
#endif

#if ISRRAN_NSSS_RETURN_PSR
  // Find second side lobe

  // Find end of peak lobe to the right
  int pl_ub = corr_peak_pos + 1;
  while (q->conv_output_abs[pl_ub + 1] <= q->conv_output_abs[pl_ub] && pl_ub < conv_output_len) {
    pl_ub++;
  }
  // Find end of peak lobe to the left
  int pl_lb;
  if (corr_peak_pos > 2) {
    pl_lb = corr_peak_pos - 1;
    while (q->conv_output_abs[pl_lb - 1] <= q->conv_output_abs[pl_lb] && pl_lb > 1) {
      pl_lb--;
    }
  } else {
    pl_lb = 0;
  }

  int sl_distance_right = conv_output_len - 1 - pl_ub;
  if (sl_distance_right < 0) {
    sl_distance_right = 0;
  }
  int sl_distance_left = pl_lb;

  int   sl_right          = pl_ub + isrran_vec_max_fi(&q->conv_output_abs[pl_ub], sl_distance_right);
  int   sl_left           = isrran_vec_max_fi(q->conv_output_abs, sl_distance_left);
  float side_lobe_value   = ISRRAN_MAX(q->conv_output_abs[sl_right], q->conv_output_abs[sl_left]);
  q->peak_values[cell_id] = q->conv_output_abs[corr_peak_pos] / side_lobe_value;
  DEBUG("NSSS n_id_ncell=%d at peak_pos=%2d, pl_ub=%2d, pl_lb=%2d, sl_right: %2d, sl_left: %2d, PSR: %.2f/%.2f=%.2f",
        cell_id,
        corr_peak_pos,
        pl_ub,
        pl_lb,
        sl_right,
        sl_left,
        q->conv_output_abs[corr_peak_pos],
        side_lobe_value,
        q->peak_values[cell_id]);
#else
  // save max. absolute value
  q->peak_values[cell_id] = q->conv_output_abs[corr_peak_pos];
  DEBUG("NSSS n_id_ncell=%d with peak=%f found at: %d", cell_id, q->peak_values[cell_id], corr_peak_pos);
#endif
}

// generate the NSSS signal for each of 4 different cyclic shifts
// return 4 * 132 = 528 complex samples
void isrran_nsss_generate(cf_t* signal, uint32_t cell_id)
{
  if (isrran_cellid_isvalid(cell_id)) {
    int u    = cell_id % 126 + 3;
    int q    = floor(cell_id / 126.0);
    int sign = -1;

    // iterate over all possible cyclic shifts
    for (int theta_f = 0; theta_f < ISRRAN_NSSS_NUM_SEQ; theta_f++) {
      for (int n = 0; n < ISRRAN_NSSS_LEN; n++) {
        int n_prime = n % 131;
        int m       = n % 128;

        float         arg = (float)sign * 2.0 * M_PI * ((float)theta_f) * ((float)n);
        float complex tmp1;
        __real__ tmp1 = cosf(arg);
        __imag__ tmp1 = sinf(arg);

        arg = ((float)sign * M_PI * ((float)u) * (float)n_prime * ((float)n_prime + 1.0)) / 131.0;
        float complex tmp2;
        __real__ tmp2 = cosf(arg);
        __imag__ tmp2 = sinf(arg);

        signal[theta_f * ISRRAN_NSSS_LEN + n] = b_q_m[q][m] * tmp1 * tmp2;
      }
    }
  } else {
    DEBUG("Invalid n_id_ncell %d", cell_id);
  }
}

void isrran_nsss_put_subframe(isrran_nsss_synch_t* q,
                              cf_t*                nsss,
                              cf_t*                subframe,
                              const int            nf,
                              const uint32_t       nof_prb,
                              const uint32_t       nbiot_prb_offset)
{
  int theta_f = (int)floor(33 / 132.0 * (nf / 2.0)) % ISRRAN_NSSS_NUM_SEQ;

  // skip first 3 OFDM symbols over all PRBs completely
  int k = 3 * nof_prb * ISRRAN_NRE + nbiot_prb_offset * ISRRAN_NRE;

  DEBUG("%d.9: Putting NSSS with theta_f=%d", nf, theta_f);
  for (int l = 0; l < ISRRAN_CP_NORM_SF_NSYMB - 3; l++) {
    memcpy(&subframe[k + ISRRAN_NSSS_NSC * l],
           &nsss[(theta_f * ISRRAN_NSSS_LEN) + (l * ISRRAN_NSSS_NSC)],
           ISRRAN_NSSS_NSC * sizeof(cf_t));
    k += (nof_prb - 1) * ISRRAN_NRE;
  }
}
