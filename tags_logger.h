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
// Author: Piaw Na (piaw@google.com)
// A wrapper for logging. We can use either google's internal logging system
// or for open source purposes, stderr
//

#ifndef TAGS_LOGGER_H
#define TAGS_LOGGER_H

#include <iostream>
#include <string>

using std::ostream;

// for holding query profile data
struct query_profile {
  std::string  client_ip;
  std::string  client;
  int     command;
  std::string  tag;
  std::string  current_file;
  std::string  client_message;
  int     time_receiving;
  int     time_searching;
  int     time_preparing_result;
  int     time_sending_result;
};

class GtagsLogger {
 public:
  virtual ostream& GetStream(const char* file, int line, int severity) = 0;
  virtual void FlushLogStream() = 0;
  virtual void Flush() = 0;
  virtual void WriteProfileData(struct query_profile *q, time_t time) = 0;
};

extern GtagsLogger* logger;

// LogStreamFlusher is used to notify the logger that we are done with streaming
// one log message and it is safe to flush data to various sinks. It does this by
// calling logger->FlushLogStream().
class LogStreamFlusher {
 public:
  // We need an operator that has higher precedence than & (which is used in similar
  // manner in CHECK macro, but lower than <<.
  // LogStreamFlusher() < logger->GetStream(...) << "log message"; will first stream
  // logging message, and then LogStreamFusher will flush the logging stream when all
  // log messages are streamed in this logging statement.
  ostream& operator< (ostream & stream) {
    logger->FlushLogStream();
    return stream;
  }
};

const int INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3, NUM_SEVERITIES = 4;

// uncomment out the standard google logger
// LOG(INFO) gets translated into:
// LogStreamFlusher() < logger->GetStream(...) << "log message".
//
// The execution order here is:
// 1. LogStreamFlusher()
// 2. logger->GetStream(...) << "log message"
// 3. operator <
//
#undef LOG
#define LOG(severity) \
LogStreamFlusher() < logger->GetStream(__FILE__, __LINE__, severity)

#undef LOG_EVERY_N
#define LOG_EVERY_N(severity, n) \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    LOG(severity)

#endif   // TAGS_LOGGER_H
