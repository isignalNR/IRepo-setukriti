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

/******************************************************************************
 * File:        metrics_csv.h
 * Description: Metrics class writing to CSV file.
 *****************************************************************************/

#ifndef ISRENB_METRICS_CSV_H
#define ISRENB_METRICS_CSV_H

#include <fstream>
#include <iostream>
#include <pthread.h>
#include <stdint.h>
#include <string>

#include "isrran/common/metrics_hub.h"
#include "isrran/interfaces/enb_metrics_interface.h"

namespace isrenb {

class metrics_csv : public isrran::metrics_listener<enb_metrics_t>
{
public:
  metrics_csv(std::string filename, enb_metrics_interface* enb_);
  ~metrics_csv();

  void set_metrics(const enb_metrics_t& m, const uint32_t period_usec);
  void stop();

private:
  std::string float_to_string(float f, int digits, bool add_semicolon = true);

  float                  metrics_report_period;
  std::ofstream          file;
  enb_metrics_interface* enb;
  uint32_t               n_reports;
};

} // namespace isrenb

#endif // ISRENB_METRICS_CSV_H
