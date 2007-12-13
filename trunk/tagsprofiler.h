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
//
// IOInterface class is a pure interface that defines  basic I/O operations that
// can be called in single tag request/response. I/O handling classes of Gtags
// should implement this interface.
//
// IOInterface class hooks into TagsIOProfiler which is call that calls calls
// Input(), runs tag_request_handler.Execute() and then calls Output(). It adds
// various timing measurements in between calls.

#ifndef TOOLS_TAGS_TAGSPROFILER_H__
#define TOOLS_TAGS_TAGSPROFILER_H__

#include "tagsutil.h"

class TagsRequestHandler;

// Interface for a class that can do some I/O operations
class IOInterface {
 public:
  // Return the name of the source.
  virtual const char* Source() const = 0;
  // Points in to a null terminated buffer.
  // Return true if there is more data to be read after the call.
  virtual bool Input(char** in) = 0;
  // Output the null terminated buffer pointed by out.
  // Return true if there is more data to be written after the call.
  virtual bool Output(const char* out) = 0;
};

// Runs tags request operation with timing measurement included
class TagsIOProfiler {
 public:
  TagsIOProfiler(IOInterface* io, TagsRequestHandler* tags_request_handler) :
      io_(io), tags_request_handler_(tags_request_handler) {}

  virtual bool Execute();

 private:
  // Returns time between start and end, in miliseconds
  int TimeBetweenClocks(clock_t end, clock_t start) {
    return static_cast<int>(
        ((static_cast<double>(end - start)) / CLOCKS_PER_SEC) * 1000);
  }

  IOInterface* io_;
  TagsRequestHandler* tags_request_handler_;
};

#endif  // TOOLS_TAGS_TAGSPROFILER_H__
