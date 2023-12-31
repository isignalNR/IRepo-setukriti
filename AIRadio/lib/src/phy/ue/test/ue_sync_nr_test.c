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

#include "isrran/common/test_common.h"
#include "isrran/phy/channel/ch_awgn.h"
#include "isrran/phy/channel/delay.h"
#include "isrran/phy/ue/ue_sync_nr.h"
#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/ringbuffer.h"
#include "isrran/phy/utils/vector.h"
#include <getopt.h>
#include <stdlib.h>

// NR parameters
static uint32_t                    pci                 = 500; // Physical Cell Identifier
static uint32_t                    carrier_nof_prb     = 52;  // Carrier bandwidth
static isrran_subcarrier_spacing_t carrier_scs         = isrran_subcarrier_spacing_15kHz;
static double                      center_frequency_hz = 3.5e9;
static isrran_subcarrier_spacing_t ssb_scs             = isrran_subcarrier_spacing_15kHz;
static double                      ssb_frequency_hz    = 3.5e9 - 960e3;
static isrran_ssb_pattern_t        ssb_pattern         = ISRRAN_SSB_PATTERN_C;
static isrran_duplex_mode_t        duplex_mode         = ISRRAN_DUPLEX_MODE_TDD;

// Test and channel parameters
static uint32_t nof_sf         = 1000;    // Number of subframes to test
static float    cfo_hz         = 100.0f;  // CFO in Hz
static float    n0_dB          = -10.0f;  // Noise floor in dB relative to full-scale
static float    delay_min_us   = 10.0f;   // Minimum dynamic delay in microseconds
static float    delay_max_us   = 1000.0f; // Maximum dynamic delay in microseconds
static float    delay_period_s = 60.0f;   // Delay period in seconds

// Test context
static double   srate_hz = 0.0;  // Base-band sampling rate
static uint32_t sf_len   = 0;    // Subframe length
static cf_t*    buffer   = NULL; // Base-band buffer
static cf_t*    buffer2  = NULL; // Base-band buffer

static void usage(char* prog)
{
  printf("Usage: %s [v]\n", prog);
  printf("\t-v [set isrran_verbose to debug, default none]\n");
}

static void parse_args(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "v")) != -1) {
    switch (opt) {
      case 'v':
        increase_isrran_verbose_level();
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
}

typedef struct {
  uint32_t               sf_idx;
  uint32_t               sfn;
  isrran_ringbuffer_t    ringbuffer;
  isrran_ssb_t           ssb;
  isrran_timestamp_t     timestamp;
  isrran_channel_awgn_t  awgn;
  isrran_channel_delay_t delay;
} test_context_t;

static void run_channel(test_context_t* ctx)
{
  // Delay
  isrran_channel_delay_execute(&ctx->delay, buffer, buffer2, sf_len, &ctx->timestamp);

  // CFO
  isrran_vec_apply_cfo(buffer2, -cfo_hz / srate_hz, buffer, sf_len);

  // AWGN
  isrran_channel_awgn_run_c(&ctx->awgn, buffer, buffer, sf_len);
}

static int test_context_init(test_context_t* ctx)
{
  ISRRAN_MEM_ZERO(ctx, test_context_t, 1);

  if (ctx == NULL) {
    return ISRRAN_ERROR_INVALID_INPUTS;
  }

  ctx->sfn = 1;

  if (isrran_ringbuffer_init(&ctx->ringbuffer, (int)(10 * sf_len * sizeof(cf_t))) < ISRRAN_SUCCESS) {
    return ISRRAN_ERROR;
  }

  isrran_ssb_args_t ssb_args = {};
  ssb_args.max_srate_hz      = srate_hz;
  ssb_args.min_scs           = ssb_scs;
  ssb_args.enable_encode     = true;
  if (isrran_ssb_init(&ctx->ssb, &ssb_args) < ISRRAN_SUCCESS) {
    return ISRRAN_ERROR;
  }

  isrran_ssb_cfg_t ssb_cfg = {};
  ssb_cfg.srate_hz         = srate_hz;
  ssb_cfg.srate_hz         = srate_hz;
  ssb_cfg.center_freq_hz   = center_frequency_hz;
  ssb_cfg.ssb_freq_hz      = ssb_frequency_hz;
  ssb_cfg.scs              = ssb_scs;
  ssb_cfg.pattern          = ssb_pattern;
  ssb_cfg.duplex_mode      = duplex_mode;
  if (isrran_ssb_set_cfg(&ctx->ssb, &ssb_cfg) < ISRRAN_SUCCESS) {
    return ISRRAN_ERROR;
  }

  if (isrran_channel_delay_init(&ctx->delay, delay_min_us, delay_max_us, delay_period_s, 0, (uint32_t)srate_hz) <
      ISRRAN_SUCCESS) {
    ERROR("Init");
    return ISRRAN_ERROR;
  }

  if (isrran_channel_awgn_init(&ctx->awgn, 0x0) < ISRRAN_SUCCESS) {
    ERROR("Init");
    return ISRRAN_ERROR;
  }

  if (isrran_channel_awgn_set_n0(&ctx->awgn, n0_dB) < ISRRAN_SUCCESS) {
    ERROR("Init");
    return ISRRAN_ERROR;
  }

  return ISRRAN_SUCCESS;
}

static void test_context_free(test_context_t* ctx)
{
  if (ctx == NULL) {
    return;
  }

  isrran_ringbuffer_free(&ctx->ringbuffer);
  isrran_ssb_free(&ctx->ssb);
  isrran_channel_delay_free(&ctx->delay);
  isrran_channel_awgn_free(&ctx->awgn);
}

static int recv_callback(void* ptr, cf_t** rx_buffer, uint32_t nof_samples, isrran_timestamp_t* timestamp)
{
  test_context_t* ctx = (test_context_t*)ptr;

  // Check inputs
  if (ctx == NULL || rx_buffer == NULL || rx_buffer[0] == NULL) {
    return ISRRAN_ERROR;
  }

  // Calculate the number of required bytes
  int required_nbytes = (int)sizeof(cf_t) * nof_samples;

  // Execute subframe until the ringbuffer has data
  while (isrran_ringbuffer_status(&ctx->ringbuffer) < required_nbytes) {
    // Reset buffer
    isrran_vec_cf_zero(buffer, sf_len);

    if (ctx->sf_idx % (ISRRAN_NOF_SF_X_FRAME / 2) == 0) {
      // Prepare PBCH message
      isrran_pbch_msg_nr_t pbch_msg = {};
      pbch_msg.ssb_idx              = 0;
      pbch_msg.hrf                  = ctx->sf_idx >= (ISRRAN_NOF_SF_X_FRAME / 2);
      pbch_msg.sfn_4lsb             = ctx->sfn & 0b1111U;

      // Encode SSB
      if (isrran_ssb_add(&ctx->ssb, pci, &pbch_msg, buffer, buffer) < ISRRAN_SUCCESS) {
        return ISRRAN_ERROR;
      }
    }

    // Run channel
    run_channel(ctx);

    // Write in the ring buffer
    if (isrran_ringbuffer_write(&ctx->ringbuffer, buffer, (int)sf_len * sizeof(cf_t)) < ISRRAN_SUCCESS) {
      return ISRRAN_ERROR;
    }

    // Increment subframe index
    ctx->sf_idx++;

    // Increment SFN if required
    if (ctx->sf_idx >= ISRRAN_NOF_SF_X_FRAME) {
      ctx->sfn    = (ctx->sfn + 1) % 1024U;
      ctx->sf_idx = 0;
    }
  }

  isrran_vec_cf_zero(buffer, sf_len);

  // Read ringbuffer
  if (isrran_ringbuffer_read(&ctx->ringbuffer, rx_buffer[0], required_nbytes) < ISRRAN_SUCCESS) {
    return ISRRAN_ERROR;
  }

  // Setup timestamp
  *timestamp = ctx->timestamp;

  // Advance timestamp
  isrran_timestamp_add(&ctx->timestamp, 0, (float)(nof_samples / srate_hz));

  return ISRRAN_SUCCESS;
}

static int test_case_1(isrran_ue_sync_nr_t* ue_sync)
{
  for (uint32_t sf_idx = 0; sf_idx < nof_sf; sf_idx++) {
    isrran_ue_sync_nr_outcome_t outcome = {};

    // Prevent buffer overflow in isrran_ue_sync_nr_zerocopy
    if (ue_sync->nof_rx_channels > 1) {
      ERROR("Error configuring number of RX channels");
      return ISRRAN_ERROR;
    }
    TESTASSERT(isrran_ue_sync_nr_zerocopy(ue_sync, &buffer, &outcome) == ISRRAN_SUCCESS);

    // Print outcome
    INFO("measure - zerocpy in-sync=%s sf_idx=%d sfn=%d timestamp=%f cfo_hz=%+.1f delay_us=%+.3f",
         outcome.in_sync ? "y" : "n",
         outcome.sf_idx,
         outcome.sfn,
         isrran_timestamp_real(&outcome.timestamp),
         outcome.cfo_hz,
         outcome.delay_us);
  }

  return ISRRAN_SUCCESS;
}

int main(int argc, char** argv)
{
  int ret = ISRRAN_ERROR;
  parse_args(argc, argv);

  if (!isnormal(srate_hz)) {
    srate_hz = (double)ISRRAN_SUBC_SPACING_NR(carrier_scs) * isrran_min_symbol_sz_rb(carrier_nof_prb);
  }
  sf_len  = (uint32_t)ceil(srate_hz / 1000.0);
  buffer  = isrran_vec_cf_malloc(sf_len);
  buffer2 = isrran_vec_cf_malloc(sf_len);

  test_context_t      ctx     = {};
  isrran_ue_sync_nr_t ue_sync = {};

  if (buffer == NULL) {
    ERROR("Malloc");
    goto clean_exit;
  }

  if (buffer2 == NULL) {
    ERROR("Malloc");
    goto clean_exit;
  }

  isrran_ue_sync_nr_args_t ue_sync_args = {};
  ue_sync_args.max_srate_hz             = srate_hz;
  ue_sync_args.min_scs                  = carrier_scs;
  ue_sync_args.recv_obj                 = &ctx;
  ue_sync_args.recv_callback            = &recv_callback;
  ue_sync_args.disable_cfo              = true;
  if (isrran_ue_sync_nr_init(&ue_sync, &ue_sync_args) < ISRRAN_SUCCESS) {
    ERROR("Init");
    goto clean_exit;
  }

  isrran_ue_sync_nr_cfg_t ue_sync_cfg = {};
  ue_sync_cfg.ssb.srate_hz            = srate_hz;
  ue_sync_cfg.ssb.center_freq_hz      = center_frequency_hz;
  ue_sync_cfg.ssb.ssb_freq_hz         = ssb_frequency_hz;
  ue_sync_cfg.ssb.scs                 = ssb_scs;
  ue_sync_cfg.ssb.pattern             = ssb_pattern;
  ue_sync_cfg.ssb.duplex_mode         = duplex_mode;
  ue_sync_cfg.N_id                    = pci;
  if (isrran_ue_sync_nr_set_cfg(&ue_sync, &ue_sync_cfg) < ISRRAN_SUCCESS) {
    ERROR("Init");
    goto clean_exit;
  }

  if (test_context_init(&ctx) < ISRRAN_SUCCESS) {
    ERROR("Init");
    goto clean_exit;
  }

  if (test_case_1(&ue_sync) != ISRRAN_SUCCESS) {
    ERROR("test case failed");
  }

  ret = ISRRAN_SUCCESS;

clean_exit:
  isrran_ue_sync_nr_free(&ue_sync);

  if (buffer) {
    free(buffer);
  }

  if (buffer2) {
    free(buffer2);
  }

  test_context_free(&ctx);

  return ret;
}
