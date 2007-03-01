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

#ifndef TOOLS_TAGS_STRUTIL_H__
#define TOOLS_TAGS_STRUTIL_H__

#ifndef GTAGS_GOOGLE_INTERNAL
#include "tagsutil.h"

// Reimplementing a few functions that we use
#include <cctype>
#include <cstring>
#include <string>

bool inline ascii_isspace(char c) { return isspace(c); }

string FastItoa(int i);

// Escape \n, \r, \t, \\, \', \" from src_string.
// Warning: not thread safe
const string & CEscape(const string & src_string);

bool IsIntToken(const char* src);

#else

#include "strings/strutil.h"

string inline FastItoa(int i) {
  return SimpleItoa(i);
}

bool IsIntToken(const char* src);

#endif  // GTAGS_GOOGLE_INTERNAL

#endif  // TOOLS_TAGS_STRUTIL_H__
