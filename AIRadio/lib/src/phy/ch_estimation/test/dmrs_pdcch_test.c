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

#include "isrran/phy/ch_estimation/dmrs_pdcch.h"
#include "isrran/phy/phch/pdcch_nr.h"
#include "isrran/support/isrran_test.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

static isrran_carrier_nr_t    carrier     = {};
static isrran_dmrs_pdcch_ce_t pdcch_ce    = {};
static uint16_t               rnti        = 0x1234;
static bool                   interleaved = false;

void usage(char* prog)
{
  printf("Usage: %s [recoIv]\n", prog);
  printf("\t-r nof_prb [Default %d]\n", carrier.nof_prb);
  printf("\t-e extended cyclic prefix [Default normal]\n");
  printf("\t-c cell_id [Default %d]\n", carrier.pci);
  printf("\t-I Enable interleaved CCE-to-REG [Default %s]\n", interleaved ? "Enabled" : "Disabled");
  printf("\t-v increase verbosity\n");
}

static void parse_args(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "recoIv")) != -1) {
    switch (opt) {
      case 'r':
        carrier.nof_prb = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'c':
        carrier.pci = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'I':
        interleaved ^= true;
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

static int run_test(isrran_dmrs_pdcch_estimator_t* estimator,
                    const isrran_coreset_t*        coreset,
                    const isrran_search_space_t*   search_space,
                    uint32_t                       aggregation_level,
                    cf_t*                          sf_symbols,
                    isrran_dmrs_pdcch_ce_t*        ce)
{
  isrran_slot_cfg_t slot_cfg = {};

  isrran_dci_location_t dci_location = {};
  dci_location.L                     = aggregation_level;

  for (slot_cfg.idx = 0; slot_cfg.idx < ISRRAN_NSLOTS_PER_FRAME_NR(carrier.scs); slot_cfg.idx++) {
    uint32_t locations[ISRRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR] = {};

    int nof_locations =
        isrran_pdcch_nr_locations_coreset(coreset, search_space, rnti, aggregation_level, slot_cfg.idx, locations);

    TESTASSERT(nof_locations == search_space->nof_candidates[aggregation_level]);

    // Prevent possible out of bounds read in locations
    TESTASSERT(nof_locations <= ISRRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR);

    for (uint32_t candidate = 0; candidate < nof_locations; candidate++) {
      dci_location.ncce = locations[candidate];

      uint32_t nof_re = carrier.nof_prb * ISRRAN_NRE * ISRRAN_NSYMB_PER_SLOT_NR;
      isrran_vec_cf_zero(sf_symbols, nof_re);

      TESTASSERT(isrran_dmrs_pdcch_put(&carrier, coreset, &slot_cfg, &dci_location, sf_symbols) == ISRRAN_SUCCESS);

      TESTASSERT(isrran_dmrs_pdcch_estimate(estimator, &slot_cfg, sf_symbols) == ISRRAN_SUCCESS);

      isrran_dmrs_pdcch_measure_t measure = {};
      TESTASSERT(isrran_dmrs_pdcch_get_measure(estimator, &dci_location, &measure) == ISRRAN_SUCCESS);

      if (fabsf(measure.rsrp - 1.0f) > 1e-2) {
        printf("EPRE=%f; RSRP=%f; CFO=%f; SYNC_ERR=%f;\n",
               measure.epre,
               measure.rsrp,
               measure.cfo_hz,
               measure.sync_error_us);
      }
      TESTASSERT(fabsf(measure.epre - 1.0f) < 1e-3f);
      TESTASSERT(fabsf(measure.rsrp - 1.0f) < 1e-3f);
      TESTASSERT(coreset->duration == 1 || fabsf(measure.cfo_hz) < 1e-3f);
      TESTASSERT(fabsf(measure.sync_error_us) < 1e-3f);

      TESTASSERT(isrran_dmrs_pdcch_get_ce(estimator, &dci_location, ce) == ISRRAN_SUCCESS);
      float avg_pow     = isrran_vec_avg_power_cf(ce->ce, ce->nof_re);
      float avg_pow_err = fabsf(avg_pow - 1.0f);
      TESTASSERT(ce->nof_re == ((ISRRAN_NRE - 3) * (1U << aggregation_level) * 6U));
      TESTASSERT(avg_pow_err < 0.1f);
    }
  }

  return ISRRAN_SUCCESS;
}

int main(int argc, char** argv)
{
  int ret = ISRRAN_ERROR;

  carrier.nof_prb = 50;

  parse_args(argc, argv);

  isrran_coreset_t coreset = {};
  if (interleaved) {
    coreset.mapping_type         = isrran_coreset_mapping_type_interleaved;
    coreset.reg_bundle_size      = isrran_coreset_bundle_size_n6;
    coreset.interleaver_size     = isrran_coreset_bundle_size_n2;
    coreset.precoder_granularity = isrran_coreset_precoder_granularity_reg_bundle;
    coreset.shift_index          = carrier.pci;

    carrier.nof_prb = 52;
    carrier.pci     = 500;
  }

  isrran_search_space_t         search_space = {};
  isrran_dmrs_pdcch_estimator_t estimator    = {};

  uint32_t nof_re     = carrier.nof_prb * ISRRAN_NRE * ISRRAN_NSYMB_PER_SLOT_NR;
  cf_t*    sf_symbols = isrran_vec_cf_malloc(nof_re);

  uint32_t test_counter = 0;
  uint32_t test_passed  = 0;

  uint32_t nof_frequency_resource = ISRRAN_MIN(ISRRAN_CORESET_FREQ_DOMAIN_RES_SIZE, carrier.nof_prb / 6);
  for (uint32_t frequency_resources = 1; frequency_resources < (1U << nof_frequency_resource); frequency_resources++) {
    uint32_t nof_freq_resources = 0;
    for (uint32_t i = 0; i < nof_frequency_resource; i++) {
      uint32_t mask             = ((frequency_resources >> i) & 1U);
      coreset.freq_resources[i] = (mask == 1);
      nof_freq_resources += mask;
    }

    for (coreset.duration = ISRRAN_CORESET_DURATION_MIN; coreset.duration <= ISRRAN_CORESET_DURATION_MAX;
         coreset.duration++) {
      // Skip case if CORESET bandwidth is not enough
      uint32_t N = isrran_coreset_get_bw(&coreset) * coreset.duration;
      if (interleaved && N % 12 != 0) {
        continue;
      }

      for (search_space.type = isrran_search_space_type_common_0; search_space.type <= isrran_search_space_type_ue;
           search_space.type++) {
        for (uint32_t i = 0; i < ISRRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR; i++) {
          uint32_t L                     = 1U << i;
          uint32_t nof_reg               = coreset.duration * nof_freq_resources * 6;
          uint32_t nof_cce               = nof_reg / 6;
          search_space.nof_candidates[i] = ISRRAN_MIN(nof_cce / L, ISRRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR);
        }

        for (uint32_t aggregation_level = 0; aggregation_level < ISRRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR;
             aggregation_level++) {
          isrran_dmrs_pdcch_estimator_init(&estimator, &carrier, &coreset);

          if (run_test(&estimator, &coreset, &search_space, aggregation_level, sf_symbols, &pdcch_ce)) {
            ERROR("Test %d failed", test_counter);
          } else {
            test_passed++;
          }
          test_counter++;
        }
      }
    }
  }

  isrran_dmrs_pdcch_estimator_free(&estimator);

  if (sf_symbols) {
    free(sf_symbols);
  }

  ret = test_passed == test_counter ? ISRRAN_SUCCESS : ISRRAN_ERROR;
  printf("%s, %d of %d test passed successfully.\n", ret ? "Failed" : "Passed", test_passed, test_counter);

  return ret;
}
