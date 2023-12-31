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

#ifndef ISRRAN_NGAP_UTILS_H
#define ISRRAN_NGAP_UTILS_H

#include "asn1_utils.h"
#include "ngap.h"
/************************
 * Forward declarations
 ***********************/

namespace asn1 {
namespace ngap {
struct rrcestablishment_cause_opts;
struct cause_radio_network_opts;
using rrcestablishment_cause_e = enumerated<rrcestablishment_cause_opts, true, 1>;
using cause_radio_network_e    = enumerated<cause_radio_network_opts, true, 2>;
} // namespace ngap
} // namespace asn1

#endif // ISRRAN_NGAP_UTILS_H