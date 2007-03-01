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
// Author: psung@google.com (Phil Sung)
//
// SymbolTable is a data structure to provide efficient storage of
// strings by only storing each unique string only once.
//
// Use the Get method to insert strings into the SymbolTable. The
// first time it is called with any particular string argument, it
// returns a newly allocated string which is equal to the argument. On
// any subsequent calls with the same argument, the return value is
// aliased to the string returned the first time.

#ifndef TOOLS_TAGS_SYMBOLTABLE_H__
#define TOOLS_TAGS_SYMBOLTABLE_H__

#include <ext/hash_set>

#include "tagsutil.h"

class SymbolTable {
 public:
  SymbolTable();

  ~SymbolTable();

  // Empties the table, deallocating all strings stored inside.
  void Clear() { FreeData(); }

  // Adds STR to the table if it's not already present, and returns a
  // pointer to it. If STR is not already in the table, then the
  // returned string is newly allocated. Otherwise, it is aliased to a
  // string which was previously created and returned.
  const char* Get(const char* str);

 private:
  class StrEq {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return !strcmp(s1, s2);
    }
  };

  // Deallocates all stored strings and empties the set.
  void FreeData();

  hash_set<const char*, hash<const char*>, StrEq>* table_;

  DISALLOW_EVIL_CONSTRUCTORS(SymbolTable);
};

#endif  // TOOLS_TAGS_SYMBOLTABLE_H__
