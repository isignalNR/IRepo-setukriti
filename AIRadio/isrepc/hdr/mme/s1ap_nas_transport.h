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
#ifndef ISREPC_S1AP_NAS_TRANSPORT_H
#define ISREPC_S1AP_NAS_TRANSPORT_H

#include "mme_gtpc.h"
#include "s1ap_common.h"
#include "isrepc/hdr/hss/hss.h"
#include "isrran/asn1/gtpc.h"
#include "isrran/asn1/s1ap.h"
#include "isrran/common/buffer_pool.h"

namespace isrepc {

class s1ap_nas_transport
{
public:
  static s1ap_nas_transport* m_instance;
  static s1ap_nas_transport* get_instance();
  static void                cleanup();
  void                       init();

  bool handle_initial_ue_message(const asn1::s1ap::init_ue_msg_s& init_ue, struct sctp_sndrcvinfo* enb_sri);
  bool handle_uplink_nas_transport(const asn1::s1ap::ul_nas_transport_s& ul_xport, struct sctp_sndrcvinfo* enb_sri);
  bool send_downlink_nas_transport(uint32_t               enb_ue_s1ap_id,
                                   uint32_t               mme_ue_s1ap_id,
                                   isrran::byte_buffer_t* nas_msg,
                                   struct sctp_sndrcvinfo enb_sri);

private:
  s1ap_nas_transport();
  virtual ~s1ap_nas_transport();

  isrlog::basic_logger& m_logger = isrlog::fetch_basic_logger("S1AP");

  s1ap* m_s1ap;

  nas_init_t m_nas_init;
  nas_if_t   m_nas_if;
};

} // namespace isrepc
#endif // ISREPC_S1AP_NAS_TRANSPORT_H
