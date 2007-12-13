// Copyright 2006 Google Inc. All Rights Reserved.
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
// Author: piaw@google.com (Piaw Na)

#ifndef TOOLS_TAGS_STDERR_LOGGER_H__
#define TOOLS_TAGS_STDERR_LOGGER_H__

#include "queryprofile.h"
#include "tagsutil.h"
#include "tags_logger.h"

class StdErrLogger : public GtagsLogger {
 public:
  StdErrLogger() { }
  ~StdErrLogger() { }

  virtual void Flush() { }

  virtual void WriteProfileData(struct query_profile *q, time_t time) {
    cerr << "\n" << "PROFILE:" << q->client_ip << "," << q->time_receiving <<
      "," << q->time_searching << "," << q->time_preparing_result << "," <<
      q->time_preparing_result << "," << q->time_sending_result << "," <<
      q->client << "," << q->tag << "," << q->command << "," <<
      q->current_file << "," << q->client_message;
  }
};

#endif  // TOOLS_TAGS_STDERR_LOGGER_H__
