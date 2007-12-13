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

// Forward declaration.
struct query_profile;

class GtagsLogger {
 public:
  virtual void Flush() = 0;
  virtual void WriteProfileData(struct query_profile *q, time_t time) = 0;
};

extern GtagsLogger* logger;

#endif   // TAGS_LOGGER_H
