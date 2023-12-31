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

#ifndef ISRRAN_DUMMY_GNB_STACK_H
#define ISRRAN_DUMMY_GNB_STACK_H

#include "dummy_rx_harq_proc.h"
#include "dummy_tx_harq_proc.h"
#include "isrenb/test/common/rlc_test_dummy.h"
#include "isrgnb/hdr/stack/common/test/dummy_nr_classes.h"
#include "isrgnb/hdr/stack/mac/mac_nr.h"
#include "isrgnb/hdr/stack/mac/sched_nr.h"
#include "isrgnb/src/stack/mac/test/sched_nr_cfg_generators.h"
#include "isrran/isrlog/isrlog.h"
#include <mutex>
#include <set>
#include <isrenb/hdr/stack/mac/common/mac_metrics.h>
#include <isrran/adt/circular_array.h>
#include <isrran/common/phy_cfg_nr.h>
#include <isrran/common/standard_streams.h>
#include <isrran/common/string_helpers.h>
#include <isrran/interfaces/gnb_interfaces.h>

class gnb_dummy_stack : public isrenb::stack_interface_phy_nr
{
  const static uint32_t NUMEROLOGY_IDX = 0;

public:
  struct prach_metrics_t {
    uint32_t count;
    float    avg_ta = 0.0f;
    float    min_ta = +INFINITY;
    float    max_ta = -INFINITY;
  };
  struct pucch_metrics_t {
    float    epre_db_avg = 0.0f;
    float    epre_db_min = +INFINITY;
    float    epre_db_max = -INFINITY;
    float    rsrp_db_avg = 0.0f;
    float    rsrp_db_min = +INFINITY;
    float    rsrp_db_max = -INFINITY;
    float    snr_db_avg  = 0.0f;
    float    snr_db_min  = +INFINITY;
    float    snr_db_max  = -INFINITY;
    float    ta_us_avg   = 0.0f;
    float    ta_us_min   = +INFINITY;
    float    ta_us_max   = -INFINITY;
    uint32_t count       = 0;
  };

  struct metrics_t {
    std::map<uint32_t, prach_metrics_t> prach           = {}; ///< PRACH metrics indexed with premable index
    isrenb::mac_ue_metrics_t            mac             = {}; ///< MAC metrics
    uint32_t                            sr_count        = 0;  ///< SR counter
    uint32_t                            cqi_count       = 0;  ///< CQI opportunity counter
    uint32_t                            cqi_valid_count = 0;  ///< Valid CQI counter
    pucch_metrics_t                     pucch           = {};
    pucch_metrics_t                     pusch           = {};
  };

private:
  isrlog::basic_logger& logger        = isrlog::fetch_basic_logger("GNB STK");
  bool                  use_dummy_mac = true;
  const uint16_t        rnti          = 0x1234;
  struct {
    isrran::circular_array<isrran_dci_location_t, ISRRAN_NOF_SF_X_FRAME> dci_location = {};
    uint32_t                                                             mcs          = 0;
    uint32_t                                                             freq_res     = 0;
    std::set<uint32_t>                                                   slots        = {};
  } dl, ul;
  isrran::circular_array<uint32_t, ISRRAN_NOF_SF_X_FRAME> dl_data_to_ul_ack;
  uint32_t                                                ss_id   = 0;
  isrran::phy_cfg_nr_t                                    phy_cfg = {};
  bool                                                    valid   = false;

  isrran::task_scheduler          task_sched;
  isrenb::rrc_nr_dummy            rrc_obj;
  isrenb::rlc_dummy               rlc_obj;
  std::unique_ptr<isrenb::mac_nr> mac;
  isrlog::basic_logger&           sched_logger;
  bool                            autofill_sch_bsr  = false;
  bool                            wait_preamble     = false;
  std::atomic<bool>               enable_user_sched = {false};

  std::mutex metrics_mutex;
  metrics_t  metrics = {};

  // HARQ feedback
  class pending_ack_t
  {
  private:
    std::mutex            mutex;
    isrran_pdsch_ack_nr_t ack = {};

  public:
    pending_ack_t() = default;
    void push_ack(isrran_harq_ack_resource_t& ack_resource)
    {
      // Prepare ACK information
      isrran_harq_ack_m_t ack_m = {};
      ack_m.resource            = ack_resource;
      ack_m.present             = true;

      std::unique_lock<std::mutex> lock(mutex);

      ack.nof_cc = 1;

      isrran_harq_ack_insert_m(&ack, &ack_m);
    }

    isrran_pdsch_ack_nr_t get_ack()
    {
      std::unique_lock<std::mutex> lock(mutex);
      isrran_pdsch_ack_nr_t        ret = ack;
      ack                              = {};
      return ret;
    }
    uint32_t get_dai()
    {
      std::unique_lock<std::mutex> lock(mutex);
      return ack.cc[0].M % 4;
    }
  };
  std::array<pending_ack_t, TTIMOD_SZ> pending_ack = {};

  // PUSCH state
  class pending_pusch_t
  {
  private:
    std::mutex          mutex;
    isrran_sch_cfg_nr_t pusch = {};
    bool                valid = false;
    uint32_t            pid   = 0;

  public:
    pending_pusch_t() = default;
    void push(const uint32_t& pid_, const isrran_sch_cfg_nr_t& pusch_)
    {
      std::unique_lock<std::mutex> lock(mutex);
      pusch = pusch_;
      pid   = pid_;
      valid = true;
    }

    bool pop(uint32_t& pid_, isrran_sch_cfg_nr_t& pusch_)
    {
      std::unique_lock<std::mutex> lock(mutex);
      bool                         ret = valid;
      pusch_                           = pusch;
      pid_                             = pid;
      valid                            = false;
      return ret;
    }
  };
  std::array<pending_pusch_t, TTIMOD_SZ> pending_pusch = {};

  dummy_tx_harq_entity tx_harq_proc;
  dummy_rx_harq_entity rx_harq_proc;

  bool schedule_pdsch(const isrran_slot_cfg_t& slot_cfg, dl_sched_t& dl_sched)
  {
    if (dl.slots.count(ISRRAN_SLOT_NR_MOD(isrran_subcarrier_spacing_15kHz, slot_cfg.idx)) == 0 or
        not enable_user_sched) {
      return true;
    }

    // Instantiate PDCCH and PDSCH
    pdcch_dl_t pdcch = {};
    pdsch_t    pdsch = {};

    // Second TB is not used
    pdsch.data[1] = nullptr;

    // Fill DCI configuration
    pdcch.dci_cfg = phy_cfg.get_dci_cfg();

    // Fill DCI context
    if (not phy_cfg.get_dci_ctx_pdsch_rnti_c(ss_id, dl.dci_location[slot_cfg.idx], rnti, pdcch.dci.ctx)) {
      logger.error("Error filling PDSCH DCI context");
      return false;
    }

    uint32_t harq_feedback     = dl_data_to_ul_ack[slot_cfg.idx];
    uint32_t harq_ack_slot_idx = TTI_ADD(slot_cfg.idx, harq_feedback);

    // Fill DCI fields
    isrran_dci_dl_nr_t& dci   = pdcch.dci;
    dci.freq_domain_assigment = dl.freq_res;
    dci.time_domain_assigment = 0;
    dci.mcs                   = dl.mcs;
    dci.rv                    = 0;
    dci.ndi                   = (slot_cfg.idx / ISRRAN_NOF_SF_X_FRAME) % 2;
    dci.pid                   = slot_cfg.idx % ISRRAN_NOF_SF_X_FRAME;
    dci.dai                   = pending_ack[harq_ack_slot_idx % pending_ack.size()].get_dai();
    dci.tpc                   = 1;
    dci.pucch_resource        = 0;
    if (dci.ctx.format == isrran_dci_format_nr_1_0) {
      dci.harq_feedback = dl_data_to_ul_ack[slot_cfg.idx] - 1;
    } else {
      dci.harq_feedback = slot_cfg.idx;
    }

    // Create PDSCH configuration
    if (not phy_cfg.get_pdsch_cfg(slot_cfg, dci, pdsch.sch)) {
      logger.error("Error converting DCI to grant");
      return false;
    }

    // Set TBS
    // Select grant and set data
    pdsch.data[0] = tx_harq_proc[slot_cfg.idx].get_tb(pdsch.sch.grant.tb[0].tbs);

    // Set softbuffer
    pdsch.sch.grant.tb[0].softbuffer.tx = &tx_harq_proc[slot_cfg.idx].get_softbuffer(dci.ndi);

    // Reset Tx softbuffer always
    isrran_softbuffer_tx_reset(pdsch.sch.grant.tb[0].softbuffer.tx);

    // Push scheduling results
    dl_sched.pdcch_dl.push_back(pdcch);
    dl_sched.pdsch.push_back(pdsch);

    // Generate PDSCH HARQ Feedback
    isrran_harq_ack_resource_t ack_resource = {};
    if (not phy_cfg.get_pdsch_ack_resource(dci, ack_resource)) {
      logger.error("Error getting ack resource");
      return false;
    }

    // Calculate PUCCH slot and push resource
    pending_ack[harq_ack_slot_idx % pending_ack.size()].push_ack(ack_resource);

    return true;
  }

  bool schedule_pusch(const isrran_slot_cfg_t& slot_cfg, dl_sched_t& dl_sched)
  {
    if (ul.slots.count(ISRRAN_SLOT_NR_MOD(isrran_subcarrier_spacing_15kHz, slot_cfg.idx + 4)) == 0 or
        not enable_user_sched) {
      return true;
    }

    // Instantiate PDCCH
    pdcch_ul_t pdcch = {};

    // Fill DCI configuration
    pdcch.dci_cfg = phy_cfg.get_dci_cfg();

    // Fill DCI context
    if (not phy_cfg.get_dci_ctx_pusch_rnti_c(ss_id, ul.dci_location[slot_cfg.idx], rnti, pdcch.dci.ctx)) {
      logger.error("Error filling PDSCH DCI context");
      return false;
    }

    // Fill DCI fields
    isrran_dci_ul_nr_t& dci   = pdcch.dci;
    dci.freq_domain_assigment = ul.freq_res;
    dci.time_domain_assigment = 0;
    dci.freq_hopping_flag     = 0;
    dci.mcs                   = ul.mcs;
    dci.rv                    = 0;
    dci.ndi                   = (slot_cfg.idx / ISRRAN_NOF_SF_X_FRAME) % 2;
    dci.pid                   = slot_cfg.idx % ISRRAN_NOF_SF_X_FRAME;
    dci.tpc                   = 1;

    // Create PDSCH configuration
    isrran_sch_cfg_nr_t pusch_cfg = {};
    if (not phy_cfg.get_pusch_cfg(slot_cfg, dci, pusch_cfg)) {
      logger.error("Error converting DCI to grant");
      return false;
    }

    // Set softbuffer
    pusch_cfg.grant.tb[0].softbuffer.rx = &rx_harq_proc[dci.pid].get_softbuffer(dci.ndi, pusch_cfg.grant.tb[0].tbs);

    // Push scheduling results
    dl_sched.pdcch_ul.push_back(pdcch);

    // Set pending PUSCH
    pending_pusch[TTI_TX(slot_cfg.idx) % pending_pusch.size()].push(dci.pid, pusch_cfg);

    return true;
  }

  bool handle_uci_data(const isrran_uci_cfg_nr_t& cfg, const isrran_uci_value_nr_t& value)
  {
    std::unique_lock<std::mutex> lock(metrics_mutex);

    // Process HARQ-ACK
    for (uint32_t i = 0; i < cfg.ack.count; i++) {
      const isrran_harq_ack_bit_t* ack_bit  = &cfg.ack.bits[i];
      bool                         is_ok    = (value.ack[i] == 1) and value.valid;
      uint32_t                     tb_count = (ack_bit->tb0 ? 1 : 0) + (ack_bit->tb1 ? 1 : 0);
      metrics.mac.tx_brate += tx_harq_proc[ack_bit->pid].get_tbs();
      metrics.mac.tx_pkts += tb_count;
      if (not is_ok) {
        metrics.mac.tx_errors += tb_count;
        logger.debug("NACK received!");
      }
    }

    // Process SR
    if (value.valid and value.sr > 0) {
      metrics.sr_count++;
    }

    // Process CQI
    for (uint32_t i = 0; i < cfg.nof_csi; i++) {
      // Increment CQI opportunity
      metrics.cqi_count++;

      // Skip if invalid or not supported CSI report
      if (not value.valid or cfg.csi[i].cfg.quantity != ISRRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI or
          cfg.csi[i].cfg.freq_cfg != ISRRAN_CSI_REPORT_FREQ_WIDEBAND) {
        continue;
      }

      // Add statistics
      metrics.mac.dl_cqi =
          ISRRAN_VEC_SAFE_CMA(value.csi->wideband_cri_ri_pmi_cqi.cqi, metrics.mac.dl_cqi, metrics.cqi_count);
      metrics.cqi_valid_count++;
    }

    return true;
  }

public:
  struct args_t {
    isrran::phy_cfg_nr_t phy_cfg;                    ///< Physical layer configuration
    std::string          use_dummy_mac = "dummymac"; ///< Use dummy or real NR scheduler
    bool     wait_preamble           = false;  ///< Whether a UE is created automatically or the stack waits for a PRACH
    uint16_t rnti                    = 0x1234; ///< C-RNTI
    uint32_t ss_id                   = 1;      ///< Search Space identifier
    uint32_t pdcch_aggregation_level = 0;      ///< PDCCH aggregation level
    uint32_t pdcch_dl_candidate      = 0;      ///< PDCCH DL DCI candidate index
    uint32_t pdcch_ul_candidate      = 1;      ///< PDCCH UL DCI candidate index
    struct {
      uint32_t    rb_start  = 0;  ///< Start frequency domain resource block
      uint32_t    rb_length = 10; ///< Number of frequency domain resource blocks
      uint32_t    mcs       = 10; ///< Modulation code scheme
      std::string slots     = ""; ///< Slot list, empty string means no scheduling
    } pdsch, pusch;
    std::string log_level = "warning";
  };

  gnb_dummy_stack(const args_t& args) :
    rnti(args.rnti),
    phy_cfg(args.phy_cfg),
    ss_id(args.ss_id),
    use_dummy_mac(args.use_dummy_mac == "dummymac"),
    sched_logger(isrlog::fetch_basic_logger("MAC")),
    wait_preamble(args.wait_preamble)
  {
    logger.set_level(isrlog::str_to_basic_level(args.log_level));
    sched_logger.set_level(isrlog::str_to_basic_level(args.log_level));
    isrlog::fetch_basic_logger("MAC-NR").set_level(isrlog::str_to_basic_level(args.log_level));

    autofill_sch_bsr = args.pdsch.slots != "" and args.pdsch.slots != "none";

    // create sched object
    mac.reset(new isrenb::mac_nr{&task_sched});
    isrenb::mac_nr_args_t mac_args{};
    mac_args.sched_cfg.pdsch_enabled = args.pdsch.slots != "" and args.pdsch.slots != "none";
    mac_args.sched_cfg.pusch_enabled = args.pusch.slots != "" and args.pusch.slots != "none";
    mac_args.sched_cfg.fixed_dl_mcs  = args.pdsch.mcs;
    mac_args.sched_cfg.fixed_ul_mcs  = args.pusch.mcs;
    mac->init(mac_args, nullptr, nullptr, &rlc_obj, &rrc_obj);
    std::vector<isrenb::sched_nr_cell_cfg_t> cells_cfg = isrenb::get_default_cells_cfg(1, phy_cfg);
    mac->cell_cfg(cells_cfg);

    dl.mcs = args.pdsch.mcs;
    ul.mcs = args.pusch.mcs;

    if (args.pdsch.slots != "none" and not args.pdsch.slots.empty()) {
      if (args.pdsch.slots == "all") {
        for (uint32_t n = 1; n < ISRRAN_NSLOTS_PER_FRAME_NR(phy_cfg.carrier.scs); n++) {
          dl.slots.insert(n);
        }
      } else {
        isrran::string_parse_list(args.pdsch.slots, ',', dl.slots);
      }
    }
    if (args.pusch.slots != "none" and not args.pusch.slots.empty()) {
      if (args.pusch.slots == "all") {
        for (uint32_t n = 0; n < ISRRAN_NSLOTS_PER_FRAME_NR(phy_cfg.carrier.scs); n++) {
          ul.slots.insert(n);
        }
      } else {
        isrran::string_parse_list(args.pusch.slots, ',', ul.slots);
      }
    }

    // Select DCI locations
    for (uint32_t slot = 0; slot < ISRRAN_NOF_SF_X_FRAME; slot++) {
      isrran::bounded_vector<isrran_dci_location_t, ISRRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR> locations;

      if (not phy_cfg.get_dci_locations(slot, rnti, args.ss_id, args.pdcch_aggregation_level, locations)) {
        logger.error(
            "Error generating locations for slot %d and aggregation level %d", slot, args.pdcch_aggregation_level);
        return;
      }

      // DCI DL
      if (args.pdcch_dl_candidate >= locations.size()) {
        logger.error("Candidate index %d exceeds the number of candidates %d for aggregation level %d",
                     args.pdcch_dl_candidate,
                     (uint32_t)locations.size(),
                     args.pdcch_aggregation_level);
        return;
      }
      dl.dci_location[slot] = locations[args.pdcch_dl_candidate];

      // DCI UL
      if (args.pdcch_ul_candidate >= locations.size()) {
        logger.error("Candidate index %d exceeds the number of candidates %d for aggregation level %d",
                     args.pdcch_ul_candidate,
                     (uint32_t)locations.size(),
                     args.pdcch_aggregation_level);
        return;
      }
      ul.dci_location[slot] = locations[args.pdcch_ul_candidate];
    }

    // Select DL frequency domain resources
    dl.freq_res = isrran_ra_nr_type1_riv(args.phy_cfg.carrier.nof_prb, args.pdsch.rb_start, args.pdsch.rb_length);

    // Select DL frequency domain resources
    ul.freq_res = isrran_ra_nr_type1_riv(args.phy_cfg.carrier.nof_prb, args.pusch.rb_start, args.pusch.rb_length);

    // Setup DL Data to ACK timing
    for (uint32_t i = 0; i < ISRRAN_NOF_SF_X_FRAME; i++) {
      if (args.phy_cfg.duplex.mode == ISRRAN_DUPLEX_MODE_TDD) {
        dl_data_to_ul_ack[i] = args.phy_cfg.harq_ack.dl_data_to_ul_ack[i % args.phy_cfg.duplex.tdd.pattern1.period_ms];
      } else {
        dl_data_to_ul_ack[i] = args.phy_cfg.harq_ack.dl_data_to_ul_ack[i % args.phy_cfg.harq_ack.nof_dl_data_to_ul_ack];
      }
    }

    // If reached this point the configuration is valid
    valid = true;
  }

  ~gnb_dummy_stack() = default;

  void stop()
  {
    if (not use_dummy_mac) {
      mac->stop();
    }
  }

  bool is_valid() const { return valid; }

  void start_scheduling()
  {
    // add UE to scheduler
    if (not use_dummy_mac and not wait_preamble) {
      isrenb::sched_nr_ue_cfg_t ue_cfg = isrenb::get_default_ue_cfg(1, phy_cfg);
      ue_cfg.lc_ch_to_add.emplace_back();
      ue_cfg.lc_ch_to_add.back().lcid          = 4;
      ue_cfg.lc_ch_to_add.back().cfg.direction = isrenb::mac_lc_ch_cfg_t::BOTH;

      mac->reserve_rnti(0, ue_cfg);
    }

    enable_user_sched = true;
  }

  int slot_indication(const isrran_slot_cfg_t& slot_cfg) override { return 0; }

  dl_sched_t* get_dl_sched(const isrran_slot_cfg_t& slot_cfg) override
  {
    logger.set_context(slot_cfg.idx);
    sched_logger.set_context(slot_cfg.idx);

    if (not use_dummy_mac) {
      if (autofill_sch_bsr) {
        mac->rlc_buffer_state(rnti, 0, 100000, 0);
        mac->ul_bsr(rnti, 0, 100000);
      }

      dl_sched_t* dl_res = mac->get_dl_sched(slot_cfg);
      if (dl_res == nullptr) {
        return nullptr;
      }

      for (pdsch_t& pdsch : dl_res->pdsch) {
        // Set TBS
        // Select grant and set data
        pdsch.data[0] = tx_harq_proc[slot_cfg.idx].get_tb(pdsch.sch.grant.tb[0].tbs);
        pdsch.data[1] = nullptr;
      }

      return dl_res;
    }
    dl_sched_t& dl_sched = dl_scheds[slot_cfg.idx];
    dl_sched             = {};

    // Check if it is TDD DL slot and PDSCH mask, if no PDSCH shall be scheduled, do not set any grant and skip
    if (not isrran_duplex_nr_is_dl(&phy_cfg.duplex, phy_cfg.carrier.scs, slot_cfg.idx)) {
      return nullptr;
    }

    if (not schedule_pdsch(slot_cfg, dl_sched)) {
      logger.error("Error scheduling PDSCH");
      return nullptr;
    }

    // Schedule SSB before UL
    for (uint32_t ssb_idx = 0; ssb_idx < ISRRAN_SSB_NOF_CANDIDATES; ssb_idx++) {
      if (phy_cfg.ssb.position_in_burst[ssb_idx]) {
        isrran_mib_nr_t mib = {};
        mib.ssb_idx         = ssb_idx;
        mib.sfn             = slot_cfg.idx / ISRRAN_NSLOTS_PER_FRAME_NR(phy_cfg.carrier.scs);
        mib.hrf             = (slot_cfg.idx % ISRRAN_NSLOTS_PER_FRAME_NR(phy_cfg.carrier.scs)) >=
                  ISRRAN_NSLOTS_PER_FRAME_NR(phy_cfg.carrier.scs) / 2;

        mac_interface_phy_nr::ssb_t ssb = {};
        if (isrran_pbch_msg_nr_mib_pack(&mib, &ssb.pbch_msg) < ISRRAN_SUCCESS) {
          logger.error("Error Packing MIB in slot %d", slot_cfg.idx);
          continue;
        }
        ssb.pbch_msg.ssb_idx = (uint32_t)ssb_idx;
        dl_sched.ssb.push_back(ssb);
      }
    }

    // Check if the UL slot is valid, if not skip UL scheduling
    if (not isrran_duplex_nr_is_ul(&phy_cfg.duplex, phy_cfg.carrier.scs, TTI_TX(slot_cfg.idx))) {
      return &dl_sched;
    }

    if (not schedule_pusch(slot_cfg, dl_sched)) {
      logger.error("Error scheduling PUSCH");
      return nullptr;
    }

    // Schedule NZP-CSI-RS, iterate all NZP-CSI-RS sets
    for (const isrran_csi_rs_nzp_set_t& set : phy_cfg.pdsch.nzp_csi_rs_sets) {
      // For each NZP-CSI-RS resource available in the set
      for (uint32_t i = 0; i < set.count; i++) {
        // Select resource
        const isrran_csi_rs_nzp_resource_t& nzp_csi_resource = set.data[i];

        // Check if the resource is scheduled for this slot
        if (isrran_csi_rs_send(&nzp_csi_resource.periodicity, &slot_cfg)) {
          dl_sched.nzp_csi_rs.push_back(nzp_csi_resource);
        }
      }
    }

    return &dl_sched;
  }

  ul_sched_t* get_ul_sched(const isrran_slot_cfg_t& slot_cfg) override
  {
    logger.set_context(slot_cfg.idx);
    sched_logger.set_context(slot_cfg.idx);

    if (not use_dummy_mac) {
      ul_sched_t* ul_res = mac->get_ul_sched(slot_cfg);
      return ul_res;
    }
    ul_sched_t& ul_sched = ul_scheds[slot_cfg.idx];
    ul_sched.pucch.clear();
    ul_sched.pusch.clear();

    // Get ACK information
    isrran_pdsch_ack_nr_t ack     = pending_ack[slot_cfg.idx % pending_ack.size()].get_ack();
    bool                  has_ack = ack.nof_cc > 0;

    if (has_ack) {
      if (logger.debug.enabled()) {
        std::array<char, 512> str = {};
        if (isrran_harq_ack_info(&ack, str.data(), (uint32_t)str.size()) > 0) {
          logger.debug("HARQ feedback:\n%s", str.data());
        }
      }
    }
    mac_interface_phy_nr::pusch_t pusch = {};
    bool has_pusch                      = pending_pusch[slot_cfg.idx % pending_pusch.size()].pop(pusch.pid, pusch.sch);

    isrran_uci_cfg_nr_t uci_cfg = {};
    if (not phy_cfg.get_uci_cfg(slot_cfg, ack, uci_cfg)) {
      logger.error("Error getting UCI configuration");
      return nullptr;
    }

    // Schedule PUSCH
    if (has_pusch) {
      // If has PUSCH, no SR shall be received
      uci_cfg.o_sr = 0;

      // Put UCI configuration in PUSCH config
      if (not phy_cfg.get_pusch_uci_cfg(slot_cfg, uci_cfg, pusch.sch)) {
        logger.error("Error setting UCI configuration in PUSCH");
        return nullptr;
      }

      ul_sched.pusch.push_back(pusch);
    } else if ((uci_cfg.ack.count > 0 || uci_cfg.nof_csi > 0 || uci_cfg.o_sr > 0) and enable_user_sched) {
      // If any UCI information is triggered, schedule PUCCH
      ul_sched.pucch.emplace_back();

      uci_cfg.pucch.rnti = rnti;

      mac_interface_phy_nr::pucch_t& pucch = ul_sched.pucch.back();
      pucch.candidates.emplace_back();
      pucch.candidates.back().uci_cfg = uci_cfg;
      if (not phy_cfg.get_pucch_uci_cfg(slot_cfg, uci_cfg, pucch.pucch_cfg, pucch.candidates.back().resource)) {
        logger.error("Error getting UCI CFG");
        return nullptr;
      }

      // If this slot has a SR opportunity and the selected PUCCH format is 1, consider positive SR.
      if (uci_cfg.o_sr > 0 and uci_cfg.ack.count > 0 and
          pucch.candidates.back().resource.format == ISRRAN_PUCCH_NR_FORMAT_1) {
        // Set SR negative
        if (uci_cfg.o_sr > 0) {
          uci_cfg.sr_positive_present = false;
        }

        // Append new resource
        pucch.candidates.emplace_back();
        pucch.candidates.back().uci_cfg = uci_cfg;
        if (not phy_cfg.get_pucch_uci_cfg(slot_cfg, uci_cfg, pucch.pucch_cfg, pucch.candidates.back().resource)) {
          logger.error("Error getting UCI CFG");
          return nullptr;
        }
      }
    }

    return &ul_sched;
  }

  int pucch_info(const isrran_slot_cfg_t& slot_cfg, const pucch_info_t& pucch_info) override
  {
    if (not use_dummy_mac) {
      mac->pucch_info(slot_cfg, pucch_info);
    } else {
      // Handle UCI data
      if (not handle_uci_data(pucch_info.uci_data.cfg, pucch_info.uci_data.value)) {
        logger.error("Error handling UCI data from PUCCH reception");
        return ISRRAN_ERROR;
      }
    }

    // Skip next steps if uci data is invalid
    if (not pucch_info.uci_data.value.valid) {
      return ISRRAN_SUCCESS;
    }

    // Handle PHY metrics
    std::unique_lock<std::mutex> lock(metrics_mutex);
    metrics.pucch.epre_db_avg = ISRRAN_VEC_CMA(pucch_info.csi.epre_dB, metrics.pucch.epre_db_avg, metrics.pucch.count);
    metrics.pucch.epre_db_min = ISRRAN_MIN(metrics.pucch.epre_db_min, pucch_info.csi.epre_dB);
    metrics.pucch.epre_db_max = ISRRAN_MAX(metrics.pucch.epre_db_max, pucch_info.csi.epre_dB);
    metrics.pucch.rsrp_db_avg = ISRRAN_VEC_CMA(pucch_info.csi.rsrp_dB, metrics.pucch.rsrp_db_avg, metrics.pucch.count);
    metrics.pucch.rsrp_db_min = ISRRAN_MIN(metrics.pucch.rsrp_db_min, pucch_info.csi.rsrp_dB);
    metrics.pucch.rsrp_db_max = ISRRAN_MAX(metrics.pucch.rsrp_db_max, pucch_info.csi.rsrp_dB);
    metrics.pucch.snr_db_avg  = ISRRAN_VEC_CMA(pucch_info.csi.snr_dB, metrics.pucch.snr_db_avg, metrics.pucch.count);
    metrics.pucch.snr_db_min  = ISRRAN_MIN(metrics.pucch.snr_db_min, pucch_info.csi.snr_dB);
    metrics.pucch.snr_db_max  = ISRRAN_MAX(metrics.pucch.snr_db_max, pucch_info.csi.snr_dB);
    metrics.pucch.ta_us_avg   = ISRRAN_VEC_CMA(pucch_info.csi.delay_us, metrics.pucch.ta_us_avg, metrics.pucch.count);
    metrics.pucch.ta_us_min   = ISRRAN_MIN(metrics.pucch.ta_us_min, pucch_info.csi.delay_us);
    metrics.pucch.ta_us_max   = ISRRAN_MAX(metrics.pucch.ta_us_max, pucch_info.csi.delay_us);
    metrics.pucch.count++;

    return ISRRAN_SUCCESS;
  }

  int pusch_info(const isrran_slot_cfg_t& slot_cfg, pusch_info_t& pusch_info) override
  {
    if (not use_dummy_mac) {
      mac->pusch_info(slot_cfg, pusch_info);
    } else {
      // Handle UCI data
      if (not handle_uci_data(pusch_info.uci_cfg, pusch_info.pusch_data.uci)) {
        logger.error("Error handling UCI data from PUCCH reception");
        return ISRRAN_ERROR;
      }

      // Handle UL-SCH metrics
      std::unique_lock<std::mutex> lock(metrics_mutex);
      if (not pusch_info.pusch_data.tb[0].crc) {
        metrics.mac.rx_errors++;
      }
      metrics.mac.rx_brate += rx_harq_proc[pusch_info.pid].get_tbs();
      metrics.mac.rx_pkts++;
    }

    // Handle PHY metrics
    metrics.pusch.epre_db_avg = ISRRAN_VEC_CMA(pusch_info.csi.epre_dB, metrics.pusch.epre_db_avg, metrics.pusch.count);
    metrics.pusch.epre_db_min = ISRRAN_MIN(metrics.pusch.epre_db_min, pusch_info.csi.epre_dB);
    metrics.pusch.epre_db_max = ISRRAN_MAX(metrics.pusch.epre_db_max, pusch_info.csi.epre_dB);
    metrics.pusch.rsrp_db_avg = ISRRAN_VEC_CMA(pusch_info.csi.rsrp_dB, metrics.pusch.rsrp_db_avg, metrics.pusch.count);
    metrics.pusch.rsrp_db_min = ISRRAN_MIN(metrics.pusch.rsrp_db_min, pusch_info.csi.rsrp_dB);
    metrics.pusch.rsrp_db_max = ISRRAN_MAX(metrics.pusch.rsrp_db_max, pusch_info.csi.rsrp_dB);
    metrics.pusch.snr_db_avg  = ISRRAN_VEC_CMA(pusch_info.csi.snr_dB, metrics.pusch.snr_db_avg, metrics.pusch.count);
    metrics.pusch.snr_db_min  = ISRRAN_MIN(metrics.pusch.snr_db_min, pusch_info.csi.snr_dB);
    metrics.pusch.snr_db_max  = ISRRAN_MAX(metrics.pusch.snr_db_max, pusch_info.csi.snr_dB);
    metrics.pusch.ta_us_avg   = ISRRAN_VEC_CMA(pusch_info.csi.delay_us, metrics.pusch.ta_us_avg, metrics.pusch.count);
    metrics.pusch.ta_us_min   = ISRRAN_MIN(metrics.pusch.ta_us_min, pusch_info.csi.delay_us);
    metrics.pusch.ta_us_max   = ISRRAN_MAX(metrics.pusch.ta_us_max, pusch_info.csi.delay_us);
    metrics.pusch.count++;

    return ISRRAN_SUCCESS;
  }

  void rach_detected(const rach_info_t& rach_info) override
  {
    if (not use_dummy_mac) {
      mac->rach_detected(rach_info);
      task_sched.run_pending_tasks();
    }

    std::unique_lock<std::mutex> lock(metrics_mutex);
    prach_metrics_t&             prach_metrics = metrics.prach[rach_info.preamble];
    prach_metrics.avg_ta = ISRRAN_VEC_SAFE_CMA((float)rach_info.time_adv, prach_metrics.avg_ta, prach_metrics.count);
    prach_metrics.min_ta = ISRRAN_MIN((float)rach_info.time_adv, prach_metrics.min_ta);
    prach_metrics.max_ta = ISRRAN_MAX((float)rach_info.time_adv, prach_metrics.max_ta);
    prach_metrics.count++;
  }

  metrics_t get_metrics()
  {
    std::unique_lock<std::mutex> lock(metrics_mutex);
    if (not use_dummy_mac) {
      isrenb::mac_metrics_t mac_metrics;
      mac->get_metrics(mac_metrics);
      metrics.mac = mac_metrics.ues[0];
    }
    return metrics;
  }

private:
  isrran::circular_array<dl_sched_t, TTIMOD_SZ> dl_scheds;
  isrran::circular_array<ul_sched_t, TTIMOD_SZ> ul_scheds;
};

#endif // ISRRAN_DUMMY_GNB_STACK_H
