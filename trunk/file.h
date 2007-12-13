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
// An abstraction of internal file routines.

#ifndef TOOLS_TAGS_FILE_H__
#define TOOLS_TAGS_FILE_H__

#include <sys/stat.h>

#include "strutil.h"

namespace gtags {

class File {
 public:
  static void Init() {}

  static bool Delete(const string& filename) {
    return (remove(filename.c_str()) == 0);
  }

  static bool Exists(const string& filename) {
    struct stat dummy;
    // stat will return 0 if the file exists
    return (stat(filename.c_str(), &dummy) == 0);
  }
};

}  // namespace gtags

#endif  // TOOLS_TAGS_FILE_H__
