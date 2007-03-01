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

#include "tagsutil.h"

const string & CEscape(const string & src_string) {
  static string buffer;
  buffer.clear();
  const char * src = src_string.c_str();

  while(*src) {
    switch (*src) {
      case '\n':
        buffer.append("\\n");
        break;
      case '\r':
        buffer.append("\\r");
        break;
      case '\t':
        buffer.append("\\t");
        break;
      case '\"':
        buffer.append("\\\"");
        break;
      case '\'':
        buffer.append("\\\'");
        break;
      case '\\':
        buffer.append("\\\\");
        break;
      default:
        buffer += *src;
    }
    src++;
  }
  return buffer;
}

// Converts an integer to a string
// Faster than sprintf("%d")
string FastItoa(int i) {
  // 10 digits and sign are good for 32-bit or smaller ints.
  // Longest is -2147483648.
  char local[11];
  char *p = local + sizeof(local);
  // Need to use uint32 instead of int32 to correctly handle
  // -2147483648.
  unsigned int n = i;
  if (i < 0)
    n = -i;
  do {
    *--p = '0' + n % 10;
    n /= 10;
  } while (n);
  if (i < 0)
    *--p = '-';
  return string(p, local + sizeof(local));
}


bool IsIntToken(const char* src) {
  // Skip sign if there is one
  if (*src == '-') {
    src++;
  }

  // An integer is at most 10 digits
  // Loop until token null terminates or counter hits 10, whichever comes first.
  short i = 0;
  while (*src && i++ < 10) {
    if (!isdigit(*src++))
      return false;
  }

  // Token is an integer if it null terminates after the loop
  // and it must be at least 1 digit
  return *src == '\0' && i > 0;
}
