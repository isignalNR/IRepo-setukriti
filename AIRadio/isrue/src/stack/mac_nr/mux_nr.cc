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

#include "isrue/hdr/stack/mac_nr/mux_nr.h"
#include "isrran/common/buffer_pool.h"
#include "isrran/interfaces/ue_rlc_interfaces.h"

namespace isrue {

mux_nr::mux_nr(mac_interface_mux_nr& mac_, isrlog::basic_logger& logger_) : mac(mac_), logger(logger_) {}

int32_t mux_nr::init(rlc_interface_mac* rlc_)
{
  rlc = rlc_;

  msg3_buff = isrran::make_byte_buffer();
  if (msg3_buff == nullptr) {
    return ISRRAN_ERROR;
  }

  rlc_buff = isrran::make_byte_buffer();
  if (rlc_buff == nullptr) {
    return ISRRAN_ERROR;
  }

  return ISRRAN_SUCCESS;
}

void mux_nr::reset()
{
  std::lock_guard<std::mutex> lock(mutex);
  this->logical_channels.clear();
}

int mux_nr::setup_lcid(const isrran::logical_channel_config_t& config)
{
  std::lock_guard<std::mutex> lock(mutex);
  return mux_base::setup_lcid(config);
}

isrran::unique_byte_buffer_t mux_nr::get_pdu(uint32_t max_pdu_len)
{
  // Lock MAC PDU from current access from PHY workers (will be moved to UL HARQ)
  std::lock_guard<std::mutex> lock(mutex);

  // initialize MAC PDU
  isrran::unique_byte_buffer_t phy_tx_pdu = isrran::make_byte_buffer();
  if (phy_tx_pdu == nullptr) {
    logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
    return nullptr;
  }

  // verify buffer is large enough for UL grant
  if (phy_tx_pdu->get_tailroom() < max_pdu_len) {
    logger.error("Can't provide MAC PDU. Grant too big (%d < %d).", phy_tx_pdu->get_tailroom(), max_pdu_len);
    return nullptr;
  }

  logger.debug("Building new MAC PDU (%d B)", max_pdu_len);
  tx_pdu.init_tx(phy_tx_pdu.get(), max_pdu_len, true);

  if (msg3_is_pending() && mac.has_crnti()) {
    tx_pdu.add_crnti_ce(mac.get_crnti());
    msg3_transmitted();
  }

  // Pack normal UL data PDU
  int32_t remaining_len = tx_pdu.get_remaing_len(); // local variable to reserve space for CEs

  if (!msg3_is_pending() && add_bsr_ce == sbsr_ce) {
    // reserve space for SBSR
    remaining_len -= 2;
  }

  // First add MAC SDUs
  for (const auto& lc : logical_channels) {
    // TODO: Add proper priority handling
    logger.debug("Adding SDUs for LCID=%d (max %d B)", lc.lcid, remaining_len);
    while (remaining_len >= MIN_RLC_PDU_LEN) {
      // read RLC PDU
      rlc_buff->clear();
      uint8_t* rd = rlc_buff->msg;

      // Determine space for RLC
      int32_t subpdu_header_len = (remaining_len >= isrran::mac_sch_subpdu_nr::MAC_SUBHEADER_LEN_THRESHOLD ? 3 : 2);

      // Read PDU from RLC (account for subPDU header)
      int pdu_len = rlc->read_pdu(lc.lcid, rd, remaining_len - subpdu_header_len);

      if (pdu_len > remaining_len) {
        logger.error("Can't add SDU of %d B. Available space %d B", pdu_len, remaining_len);
        break;
      } else {
        // Add SDU if RLC has something to tx
        if (pdu_len > 0) {
          rlc_buff->N_bytes = pdu_len;
          logger.debug(rlc_buff->msg, rlc_buff->N_bytes, "Read %d B from RLC", rlc_buff->N_bytes);

          // add to MAC PDU and pack
          if (tx_pdu.add_sdu(lc.lcid, rlc_buff->msg, rlc_buff->N_bytes) != ISRRAN_SUCCESS) {
            logger.error("Error packing MAC PDU");
            break;
          }

          if (lc.lcid == 0 && msg3_is_pending()) {
            // TODO:
            msg3_transmitted();
          }

        } else {
          // couldn't read PDU from RLC
          break;
        }

        remaining_len -= (pdu_len + subpdu_header_len);
        logger.debug("%d B remaining PDU", remaining_len);
      }
    }
  }

  // check if
  if (add_bsr_ce == no_bsr) {
    // tell BSR proc we still have space in PDU and let it decide to create a padding BSR
    mac.set_padding_bytes(tx_pdu.get_remaing_len());
  }

  // Second add fixed-sized MAC CEs (e.g. SBSR)
  if (add_bsr_ce == sbsr_ce) {
    tx_pdu.add_sbsr_ce(mac.generate_sbsr());
    add_bsr_ce = no_bsr;
  } else if (add_bsr_ce == lbsr_ce) {
    // TODO: implement LBSR support
    tx_pdu.add_sbsr_ce(mac.generate_sbsr());
    add_bsr_ce = no_bsr;
  }

  // Lastly, add variable-sized MAC CEs

  // Pack PDU
  tx_pdu.pack();

  if (logger.info.enabled()) {
    // log pretty printed PDU
    fmt::memory_buffer buff;
    tx_pdu.to_string(buff);
    logger.info("%s", isrran::to_c_str(buff));
    logger.debug(phy_tx_pdu->msg, phy_tx_pdu->N_bytes, "Generated MAC PDU (%d B)", phy_tx_pdu->N_bytes);
  }

  return phy_tx_pdu;
}

void mux_nr::msg3_flush()
{
  msg3_buff->clear();
  msg3_state = msg3_state_t::none;
}

void mux_nr::msg3_prepare()
{
  msg3_state = msg3_state_t::pending;
}

void mux_nr::msg3_transmitted()
{
  msg3_state = msg3_state_t::transmitted;
}

bool mux_nr::msg3_is_transmitted()
{
  return msg3_state == msg3_state_t::transmitted;
}

bool mux_nr::msg3_is_pending()
{
  return msg3_state == msg3_state_t::pending;
}

bool mux_nr::msg3_is_empty()
{
  return msg3_buff->N_bytes == 0;
}

void mux_nr::generate_bsr_mac_ce(const isrran::bsr_format_nr_t& format)
{
  switch (format) {
    case isrran::SHORT_BSR:
      add_bsr_ce = sbsr_ce;
      break;
    case isrran::LONG_BSR:
      add_bsr_ce = lbsr_ce;
      break;
    default:
      logger.error("MUX can only be instructred to generate short or long BSRs.");
  }
}

} // namespace isrue
