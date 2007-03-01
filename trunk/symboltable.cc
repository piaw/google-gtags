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

#include <ext/hash_set>
#include <utility>

#include "symboltable.h"

SymbolTable::SymbolTable()
    : table_(new hash_set<const char*, hash<const char*>, StrEq>()) { }

SymbolTable::~SymbolTable() {
  FreeData();
  delete table_;
}

const char* SymbolTable::Get(const char* str)  {
  int length = strlen(str);
  hash_set<const char*, hash<const char*>, StrEq>::const_iterator elt
    = table_->find(str);

  if (elt == table_->end()) {
    // If STR isn't already in table, copy and insert it
    char* str_copy = new char[length + 1];
    strncpy(str_copy, str, length);
    str_copy[length] = '\0';

    pair<hash_set<const char*, hash<const char*>, StrEq>::const_iterator,
      bool> insert_result = table_->insert(str_copy);
    CHECK(insert_result.second);
    return str_copy;
  } else {
    return *elt;
  }
}

void SymbolTable::FreeData() {
  hash_set<const char*, hash<const char*>, StrEq>::iterator iter
    = table_->begin();
  while (iter != table_->end()) {
    // Deleting an element invalidates iterators that point at that
    // element, so we need to advance the iterator before we delete
    // the element. This is the same as STLDeleteContainerPointers
    // except that it uses delete[] instead of delete.
    hash_set<const char*, hash<const char*>, StrEq>::iterator temp = iter;
    ++iter;
    delete[] *temp;
  }

  table_->clear();
}
