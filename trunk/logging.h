// Copyright 2006  Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Author: Nigel D'souza (nigdsouza@google.com)
//
// A logging wrapper that defaults to the internal base/logging.h but defines
// stderr alternatives for opensourcing.

#ifndef TOOLS_TAGS_LOGGING_H__
#define TOOLS_TAGS_LOGGING_H__

#include <iostream>
#include <string>

// LogStreamer is used to add a new-line at the end of LOGs.
class LogStreamer {
 public:
  LogStreamer(const char* file, int line, int severity)
      : stream_(std::cerr) {
    const std::string ERROR_LEVELS[] = { "INFO", "WARNING", "ERROR", "FATAL" };
    stream_ << ERROR_LEVELS[severity] << ":"
            << file << ":" << line << "| ";
  }
  virtual ~LogStreamer() {
    stream_ << std::endl;
  }

  std::ostream& stream() { return stream_; }

 protected:
  std::ostream &stream_;
};

const int INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3, NUM_SEVERITIES = 4;

// We wrap a call to LOG within an instance of LogStreamer.  At the end of the
// call to LOG, LogStreamer is automatically destroyed, hence adding the
// new-line.  We now take advantage of the destructor being the last thing to
// execute.
#define LOG(severity) \
LogStreamer(__FILE__, __LINE__, severity).stream()

#define LOG_EVERY_N(severity, n) \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    LOG(severity)

#endif   // TOOLS_TAGS_LOGGING_H
