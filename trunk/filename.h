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
// Authors: leandrog@google.com (Leandro Groisman), psung@google.com (Phil Sung)
//
// Filename stores a representation of a pathname. These are supposed
// to be paths relative to some source tree root, so they should look
// like "tools/tags/file.cc" (no leading slash).
//
// An input string containing a directory should have a trailing
// slash. We normalize the input filename by removing directories
// named "." (so that "./tools/tags/" and "tools/tags" evaluate to be
// equal). The exception to the previous two rules is that the root
// directory itself is canonically represented as ".".
//
// If a SymbolTable is provided during construction, we store all
// needed strings inside the StringTable. Otherwise, we take
// responsibility for allocating and deallocating the strings needed
// for the path.

#ifndef TOOLS_TAGS_FILENAME_H__
#define TOOLS_TAGS_FILENAME_H__

#include "symboltable.h"

class Filename {
 public:
  // New Filename representing FILE given as a string; use SYM to
  // store needed strings.
  Filename(const char* file, SymbolTable* sym);

  // New Filename representing FILE given as a string; don't use a
  // SymbolTable, but allocate (and be responsible for deleting) any
  // needed strings.
  explicit Filename(const char* file);

  // Copy constructor
  explicit Filename(const Filename& rhs);

  ~Filename();

  // Less-than operator using lexicographic ordering on the components
  // of each filename.
  bool operator<(const Filename& f) const;

  // Equality operator using equality on the filename_ array
  bool operator==(const Filename& f) const;
  bool operator!=(const Filename& f) const;

  // Calculates the directory distance between two files, defined as
  // the number of 'cd ..' or 'cd <immediate-child>' commands it takes
  // to get from the containing directory of one to the containing
  // directory of the other.
  int DistanceTo(const Filename& f) const;

  // Returns a normalized string representation of the path
  string Str() const;

  // Returns a string containing the file basename, i.e. the last
  // non-empty component. Returns NULL if the path has no components
  // (i.e. it's equal to "."). If the return value is not null, the
  // string is guaranteed to be one of the components of the current
  // path (and therefore has a lifetime of at least as long as this
  // Filename object).
  const char* Basename() const;

 private:
  // Initializes the members with the given file path
  void Initialize(const char* file);

  // Allocates a new string or gets it from a SymbolTable, depending
  // on whether we are using a SymbolTable. In either case, returns a
  // string which is streq to STR but not ==.
  inline const char* GetString(const char* str) {
    if (symboltable_)
      return symboltable_->Get(str);

    char* newstr = new char[strlen(str) + 1];
    strncpy(newstr, str, strlen(str) + 1);
    return newstr;
  }

  // Pointer to symbol table to use. If NULL, if we're supposed to
  // allocate our own strings.
  SymbolTable* const symboltable_;

  // Array of filenames
  const char* const * filename_;
  // Size of filename array
  int subdirs_;
};

#endif  // TOOLS_TAGS_FILENAME_H__
