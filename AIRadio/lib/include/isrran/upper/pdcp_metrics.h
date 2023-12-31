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

#ifndef ISRRAN_PDCP_METRICS_H
#define ISRRAN_PDCP_METRICS_H

#include "isrran/common/common.h"
#include <iostream>

namespace isrran {

typedef struct {
  // PDU metrics
  uint32_t num_tx_pdus;
  uint32_t num_rx_pdus;
  uint64_t num_tx_pdu_bytes;
  uint64_t num_rx_pdu_bytes;

  // ACK specific metrics (requires RLC AM)
  uint64_t num_tx_acked_bytes;         //< Cumulative number of bytes that the PDCP knows to be acknowledged
  uint64_t tx_notification_latency_ms; //< Average time in ms from PDU delivery to RLC to ACK notification from RLC
  uint32_t num_tx_buffered_pdus;       //< Number of PDUs waiting for ACK
  uint32_t num_tx_buffered_pdus_bytes; //< Number of bytes of PDUs waiting for ACK
} pdcp_bearer_metrics_t;

typedef struct {
  pdcp_bearer_metrics_t bearer[ISRRAN_N_RADIO_BEARERS];
} pdcp_metrics_t;

} // namespace isrran

#endif // ISRRAN_RLC_METRICS_H
