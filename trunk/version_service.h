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
// Author: nigdsouza@google.com (Nigel D'souza)
//
// A wrapper interface for VersionService.

#ifndef TOOLS_TAGS_VERSION_SERVICE_H__
#define TOOLS_TAGS_VERSION_SERVICE_H__

#include "thread.h"

namespace gtags {

class VersionServiceProvider : public Thread {
 public:
  VersionServiceProvider(int port, int version) :
      port_(port), version_(version), servicing_(false) {}
  virtual ~VersionServiceProvider() {}

  virtual bool servicing() { return servicing_; }

 protected:
  int port_;
  int version_;
  bool servicing_;
};

class VersionServiceUser {
 public:
  VersionServiceUser(int port) : port_(port) {}
  virtual ~VersionServiceUser() {}

  virtual bool GetVersion(int *version) = 0;
  virtual bool ShutDown() = 0;

 protected:
  int port_;
};

}  // namespace gtags

#endif  // TOOLS_TAGS_VERSION_SERVICE_H__
