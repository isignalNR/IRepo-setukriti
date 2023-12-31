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

#ifndef ISRRAN_ENB_COMMAND_INTERFACE_H
#define ISRRAN_ENB_COMMAND_INTERFACE_H

#include <cstdint>

namespace isrenb {
class enb_command_interface
{
public:
  /**
   * Trigger downlink singnal measurements (currently PAPR)
   */
  virtual void cmd_cell_measure() = 0;

  /**
   * Sets the relative gain of a cell from it's index (following rr.conf) order.
   * @param cell_id Provides a cell identifier
   * @param gain Relative gain
   */
  virtual void cmd_cell_gain(uint32_t cell_id, float gain) = 0;

  virtual void toggle_padding() = 0;
};
} // namespace isrenb

#endif // ISRRAN_ENB_COMMAND_INTERFACE_H
