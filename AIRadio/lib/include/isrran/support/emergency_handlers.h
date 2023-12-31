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

#ifndef ISRRAN_EMERGENCY_HANDLERS_H
#define ISRRAN_EMERGENCY_HANDLERS_H

using emergency_cleanup_callback = void (*)(void*);

// Add a cleanup function to be called when a kill signal is about to be delivered to the process. The handler may
// optionally pass a pointer to identify what instance of the handler is being called.
// Returns the id of the handler as a positive value, otherwise returns -1 on error.
int add_emergency_cleanup_handler(emergency_cleanup_callback callback, void* data);

// Removes the emergency handler with the specified id.
void remove_emergency_cleanup_handler(int id);

// Executes all registered emergency cleanup handlers.
void execute_emergency_cleanup_handlers();

#endif // ISRRAN_EMERGENCY_HANDLERS_H
