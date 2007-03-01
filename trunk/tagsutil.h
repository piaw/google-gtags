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

#ifndef TOOLS_TAGS_TAGSUTIL_H__
#define TOOLS_TAGS_TAGSUTIL_H__

#include "tags_logger.h"

using namespace std;
using namespace __gnu_cxx;

#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(type_name) \
type_name(const type_name &);\
void operator=(const type_name &)

namespace {
// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageExiter {
 public:
  LogMessageExiter() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(ostream& o) { o.flush(); exit(-1); }
};

}

#undef CHECK
#define CHECK(condition) \
(condition) ? (void) 0 : LogMessageExiter() & LOG(FATAL) << "CHECK Failed: " #condition

#undef CHECK_NE
#define CHECK_NE(a, b) \
CHECK((a) != (b))

#undef CHECK_EQ
#define CHECK_EQ(a, b) \
CHECK ((a) == (b))

#undef CHECK_GE
#define CHECK_GE(a, b) \
CHECK ((a) >= (b))

#undef CHECK_STRNE
#define CHECK_STRNE(s1, s2) \
CHECK(strcmp(s1, s2) != 0)

typedef long long int64;

template<typename To, typename From>
To down_cast(From * from) {
  To to = dynamic_cast<To>(from);
  CHECK(from == NULL || to != NULL);
  return to;
}

template<typename To, typename From>
To implicit_cast(From * from) {
  return from;
}

#endif  // TOOLS_TAGS_TAGSUTIL_H__
