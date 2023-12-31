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
#include "isrran/phy/io/filesource.h"
#include "isrran/phy/sync/ssb.h"
#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/vector.h"
#include <complex.h>
#include <getopt.h>
#include <stdlib.h>

// NR parameters
static isrran_ssb_pattern_t        ssb_pattern = ISRRAN_SSB_PATTERN_C;
static isrran_subcarrier_spacing_t ssb_scs     = isrran_subcarrier_spacing_30kHz;
static isrran_duplex_mode_t        duplex_mode = ISRRAN_DUPLEX_MODE_TDD;

// Test context
static char*    filename       = NULL;
static double   srate_hz       = 23.04e6; // Base-band sampling rate in Hz
static double   center_freq_hz = NAN;     // Center frequency in Hz
static double   ssb_freq_hz    = NAN;     // SSB frequency in Hz
static uint32_t nof_samples    = 0;       // Number of samples

// Assertion
static bool     assert          = false;
static uint32_t assert_pci      = 0;
static uint32_t assert_t_offset = 0;
static uint32_t assert_sfn_lsb  = 0;
static uint32_t assert_ssb_idx  = 0;
static uint32_t assert_ssb_k    = 0;
static uint32_t assert_hrf      = 0;

static void usage(char* prog)
{
  printf("Usage: %s -i filename [rv]\n", prog);
  printf("\t-r sampling rate in Hz [Default %.2f MHz]\n", srate_hz / 1e6);
  printf("\t-n number of samples [Default %d]\n", nof_samples);
  printf("\t-s SSB subcarrier spacing (15, 30) [Default %s]\n", isrran_subcarrier_spacing_to_str(ssb_scs));
  printf("\t-d duplex mode [Default %s]\n", duplex_mode == ISRRAN_DUPLEX_MODE_FDD ? "FDD" : "TDD");
  printf("\t-f absolute baseband center frequency in Hz [Default %.2f MHz]\n", center_freq_hz / 1e3);
  printf("\t-F absolute SSB center freuqency in Hz [Default %.2f MHz]\n", ssb_freq_hz / 1e3);
  printf("\t-A Assert: PCI t_offset sfn_lsb ssb_idx ssb_k hrf");
  printf("\t-v [set isrran_verbose to debug, default none]\n");
}

static void parse_args(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "insdrfFAv")) != -1) {
    switch (opt) {
      case 'i':
        filename = argv[optind];
        break;
      case 'n':
        nof_samples = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 's':
        if ((uint32_t)strtol(argv[optind], NULL, 10) == 15) {
          ssb_scs = isrran_subcarrier_spacing_15kHz;
        } else {
          ssb_scs = isrran_subcarrier_spacing_30kHz;
        }
        break;
      case 'd':
        if (strcmp(argv[optind], "tdd") == 0) {
          duplex_mode = ISRRAN_DUPLEX_MODE_TDD;
        } else if (strcmp(argv[optind], "fdd") == 0) {
          duplex_mode = ISRRAN_DUPLEX_MODE_FDD;
        } else {
          printf("Invalid duplex mode '%s'\n", argv[optind]);
          usage(argv[0]);
          exit(-1);
        }
        break;
      case 'r':
        srate_hz = strtod(argv[optind], NULL);
        break;
      case 'f':
        center_freq_hz = strtod(argv[optind], NULL);
        break;
      case 'F':
        ssb_freq_hz = strtod(argv[optind], NULL);
        break;
      case 'A':
        assert          = true;
        assert_pci      = (uint32_t)strtol(argv[optind++], NULL, 10);
        assert_t_offset = (uint32_t)strtol(argv[optind++], NULL, 10);
        assert_sfn_lsb  = (uint32_t)strtol(argv[optind++], NULL, 10);
        assert_ssb_idx  = (uint32_t)strtol(argv[optind++], NULL, 10);
        assert_ssb_k    = (uint32_t)strtol(argv[optind++], NULL, 10);
        assert_hrf      = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'v':
        increase_isrran_verbose_level();
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
}

static int assert_meas(uint32_t N_id, const isrran_csi_trs_measurements_t* res)
{
  TESTASSERT(N_id == assert_pci);
  return ISRRAN_SUCCESS;
}
static int assert_search(const isrran_ssb_search_res_t* res)
{
  TESTASSERT(res->N_id == assert_pci);
  TESTASSERT(res->t_offset == assert_t_offset);
  TESTASSERT(res->pbch_msg.sfn_4lsb == assert_sfn_lsb);
  TESTASSERT(res->pbch_msg.ssb_idx == assert_ssb_idx);
  TESTASSERT(res->pbch_msg.k_ssb_msb == assert_ssb_k);
  TESTASSERT((res->pbch_msg.hrf ? 1 : 0) == assert_hrf);
  return ISRRAN_SUCCESS;
}

int main(int argc, char** argv)
{
  isrran_filesource_t filesource = {};
  isrran_ssb_t        ssb        = {};
  int                 ret        = ISRRAN_ERROR;
  parse_args(argc, argv);

  if (nof_samples == 0 || !isnormal(ssb_freq_hz) || !isnormal(center_freq_hz)) {
    ERROR("Invalid arguments!");
    usage(argv[0]);
    return ISRRAN_ERROR;
  }

  cf_t* buffer = isrran_vec_cf_malloc(nof_samples);
  if (buffer == NULL) {
    ERROR("Malloc");
    goto clean_exit;
  }

  // Initialise SSB
  isrran_ssb_args_t ssb_args = {};
  ssb_args.enable_decode     = true;
  ssb_args.enable_search     = true;
  if (isrran_ssb_init(&ssb, &ssb_args) < ISRRAN_SUCCESS) {
    ERROR("Init");
    goto clean_exit;
  }

  // Configure SSB
  isrran_ssb_cfg_t ssb_cfg = {};
  ssb_cfg.srate_hz         = srate_hz;
  ssb_cfg.center_freq_hz   = center_freq_hz;
  ssb_cfg.ssb_freq_hz      = ssb_freq_hz;
  ssb_cfg.scs              = ssb_scs;
  ssb_cfg.pattern          = ssb_pattern;
  ssb_cfg.duplex_mode      = duplex_mode;
  if (isrran_ssb_set_cfg(&ssb, &ssb_cfg) < ISRRAN_SUCCESS) {
    ERROR("Error setting SSB configuration");
    goto clean_exit;
  }

  // Initialise file source
  if (isrran_filesource_init(&filesource, filename, ISRRAN_COMPLEX_FLOAT_BIN) < ISRRAN_SUCCESS) {
    ERROR("Error opening file");
    goto clean_exit;
  }

  // Read baseband
  if (isrran_filesource_read(&filesource, buffer, (int)nof_samples) < ISRRAN_SUCCESS) {
    ERROR("Error reading from file");
    goto clean_exit;
  }

  // Perform SSB-CSI Search
  uint32_t                      N_id = 0;
  isrran_csi_trs_measurements_t meas = {};
  if (isrran_ssb_csi_search(&ssb, buffer, nof_samples, &N_id, &meas) < ISRRAN_SUCCESS) {
    ERROR("Error performing SSB-CSI search");
    goto clean_exit;
  }

  // Print measurement
  char str[512] = {};
  isrran_csi_meas_info(&meas, str, sizeof(str));
  INFO("measure - search pci=%d %s", N_id, str);

  // Assert measurement
  if (assert) {
    if (assert_meas(N_id, &meas)) {
      ERROR("Error asserting search");
      goto clean_exit;
    }
  }

  // Perform SSB search
  isrran_ssb_search_res_t search_res = {};
  if (isrran_ssb_search(&ssb, buffer, nof_samples, &search_res) < ISRRAN_SUCCESS) {
    ERROR("Error performing SSB search");
    goto clean_exit;
  }

  // Print decoded PBCH message
  isrran_pbch_msg_info(&search_res.pbch_msg, str, sizeof(str));
  INFO("search - t_offset=%d pci=%d %s crc=%s",
       search_res.t_offset,
       search_res.N_id,
       str,
       search_res.pbch_msg.crc ? "OK" : "KO");

  // unpack MIB
  isrran_mib_nr_t mib = {};
  if (isrran_pbch_msg_nr_mib_unpack(&search_res.pbch_msg, &mib) < ISRRAN_SUCCESS) {
    ERROR("Error unpacking PBCH-MIB");
    goto clean_exit;
  }

  char mib_info[512] = {};
  isrran_pbch_msg_nr_mib_info(&mib, mib_info, sizeof(mib_info));
  INFO("PBCH-MIB: %s", mib_info);

  // Assert search
  if (assert) {
    if (assert_search(&search_res)) {
      ERROR("Error asserting search");
      goto clean_exit;
    }
  }

  ret = ISRRAN_SUCCESS;

clean_exit:
  isrran_ssb_free(&ssb);
  isrran_filesource_free(&filesource);

  if (buffer) {
    free(buffer);
  }

  return ret;
}