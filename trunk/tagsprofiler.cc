// Copyright 2007 Google Inc. All Rights Reserved.
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
// Author: stephenchen@google.com (Stephen Chen)

#include <string>
#include <time.h>

#include "queryprofile.h"
#include "tagsprofiler.h"
#include "tagsrequesthandler.h"
#include "tags_logger.h"

bool TagsIOProfiler::Execute() {
  clock_t clock_before_receiving = 0, clock_before_searching = 0,
  clock_before_preparing_results = 0, clock_before_sending_results = 0,
     clock_after_sending_results = 0;
  // To log performance we use the ctime library. clock() gives you
  // a timestamp in 'ticks', (end - start) / CLOCKS_PER_SECOND is
  // the formula to calculate the distance (in seconds) between two
  // 'clock()' timestamps.

  clock_before_receiving  = clock();
  char * input = 0;
  if (io_->Input(&input)) {
    return true;
  }

  if (!input) {
    return false;
  }

  clock_before_searching = clock();
  struct query_profile q;
  string output =
    tags_request_handler_->Execute(input,
                                   &clock_before_preparing_results,
                                   &q);

  clock_before_sending_results = clock();
  io_->Output(output.c_str());

  logger->Flush();
  clock_after_sending_results = clock();

  q.client_ip = io_->Source();
  q.time_receiving = TimeBetweenClocks(clock_before_searching,
                                       clock_before_receiving);
  q.time_searching = TimeBetweenClocks(clock_before_preparing_results,
                                       clock_before_searching);
  q.time_preparing_result = TimeBetweenClocks(clock_before_sending_results,
                                              clock_before_preparing_results);
  q.time_sending_result = TimeBetweenClocks(clock_after_sending_results,
                                            clock_before_sending_results);

  time_t now = time(NULL);
  logger->WriteProfileData(&q, now);
  return false;
}
