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

// Reimplementing a few functions that we use
#include <cctype>
#include <cstring>
#include <string>

using std::string;

bool inline ascii_isspace(char c) { return isspace(c); }

string FastItoa(int i);

// Escape \n, \r, \t, \\, \', \" from src_string.
// Warning: not thread safe
const string & CEscape(const string & src_string);

inline bool HasPrefixString(const string &str, const string &prefix) {
  return str.compare(0, prefix.length(), prefix) == 0;
}

inline const char* var_strprefix(const char* str, const char* prefix) {
  const int len = strlen(prefix);
  return strncmp(str, prefix, len) == 0 ?  str + len : NULL;
}

// Declare a hash functions for strings to allow us to use hash_* containers
// with strings.
namespace __gnu_cxx {
template<typename T> struct hash;

template<>
struct hash<string> {
  size_t operator() (const string& s) const;
};
}

bool IsIntToken(const char* src);

#endif  // TOOLS_TAGS_STRUTIL_H__
