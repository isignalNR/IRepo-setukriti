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

#ifndef ISRUE_TTCN3_SWAPPABLE_SINK_H
#define ISRUE_TTCN3_SWAPPABLE_SINK_H

#include "isrran/isrlog/sink.h"

/// A custom sink implementation that allows hot swapping file sinks so that loggers can write to different files
/// dynamically or other stream-like sinks like stdout.
class swappable_sink : public isrlog::sink
{
public:
  swappable_sink(const std::string& filename, std::unique_ptr<isrlog::log_formatter> f) :
    isrlog::sink(std::move(f)), s(&isrlog::fetch_file_sink(filename))
  {}

  /// Identifier of this custom sink.
  static const char* name() { return "swappable_sink"; }

  isrlog::detail::error_string write(isrlog::detail::memory_buffer buffer) override { return s->write(buffer); }

  isrlog::detail::error_string flush() override { return s->flush(); }

  /// Swaps the current file sink with a new sink that will write to the specified file name.
  void swap_sink(const std::string& filename)
  {
    isrlog::flush();
    s = &isrlog::fetch_file_sink(filename);
  }

  /// Swaps the current sink to use the stdout sink.
  void swap_to_stdout()
  {
    isrlog::flush();
    s = &isrlog::fetch_stdout_sink();
  }

private:
  isrlog::sink* s;
};

#endif // ISRUE_TTCN3_SWAPPABLE_SINK_H
