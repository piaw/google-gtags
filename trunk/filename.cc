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
// Internally, we store the pathname as an array of char*s, each of
// which stores a single directory component.

#include <algorithm>
#include <vector>
#include <string>

#include "filename.h"

// We store each path as a sequence of char* containing the parent
// directory names and the file basename-- exactly what you would get
// if you split the input by "/". By convention (and to make
// directory-distance calculations easier), the representation of a
// directory has an empty string after the directory basename. (This
// is just what you would get if you split the input by "/", assuming
// that the input had a trailing slash.)
Filename::Filename(const char* file, SymbolTable* sym) : symboltable_(sym) {
  CHECK_NE(sym, static_cast<SymbolTable*>(NULL))
    << "NULL SymbolTable is not allowed!";
  Initialize(file);
}

Filename::Filename(const char* file) : symboltable_(NULL) {
  Initialize(file);
}

Filename::Filename(const Filename& rhs)
    : symboltable_(rhs.symboltable_), subdirs_(rhs.subdirs_) {
  const char ** filetmp = new const char*[rhs.subdirs_];
  filename_ = filetmp;
  for (int i = 0; i < rhs.subdirs_; ++i, ++filetmp)
    *filetmp = GetString(rhs.filename_[i]);
}

Filename::~Filename() {
  // Delete strings only if they are not being stored in a SymbolTable
  // (i.e. we allocated them ourselves in the ctor)
  if (symboltable_ == NULL) {
    for (int i = 0; i < subdirs_; ++i)
      delete[] filename_[i];
  }

  delete[] filename_;
}

bool Filename::operator<(const Filename& f) const {
  const char * const * file1 = this->filename_;
  const char * const * file2 = f.filename_;

  // At the beginning of each loop iteration, min_subdirs is always
  // the number of unchecked indices which are present in both
  // Filenames.
  int min_subdirs = min(this->subdirs_, f.subdirs_);

  while (min_subdirs > 0) {
    // If two directories at the same index compare unequal, we can
    // return a result here.
    int compare = strcmp(*file1, *file2);
    if (compare != 0)
      return compare < 0;
    file1++;
    file2++;
    --min_subdirs;
  }

  // The two files are equal up to the length of the shorter one, so
  // figure out which is longer.
  return this->subdirs_ < f.subdirs_;
}

bool Filename::operator==(const Filename& rhs) const {
  if (rhs.subdirs_ != this->subdirs_)
    return false;
  for (int i = 0; i < rhs.subdirs_; ++i)
    if (strcmp(rhs.filename_[i], this->filename_[i]) != 0)
      return false;
  return true;
}

bool Filename::operator!=(const Filename& rhs) const {
  return !(*this == rhs);
}

int Filename::DistanceTo(const Filename& f) const {
  const char * const * file1 = f.filename_;
  const char * const * file2 = this->filename_;

  int min_length = min(f.subdirs_, this->subdirs_) - 1;
  int common_subdirs = 0;

  while (common_subdirs < min_length) {
    // TODO(psung): Directory distance computations could be faster if
    // we do pointer comparison when the symbol tables are equal. This
    // might be worth profiling. Same goes for operator==.
    if (strcmp(*file1, *file2) != 0)
      break;
    ++file1;
    ++file2;
    ++common_subdirs;
  }

  return f.subdirs_ + this->subdirs_ - 2*common_subdirs - 2;
}

string Filename::Str() const {
  string s;

  for (int i = 0; i < subdirs_; ++i) {
    if (i > 0)
      s.push_back('/');
    s.append(filename_[i]);
  }

  if (s.length() == 0)
    return ".";
  else
    return s;
}

const char* Filename::Basename() const {
  int index = subdirs_ - 1;

  while (index >= 0 && strlen(filename_[index]) == 0)
    --index;

  if (index >= 0)
    return filename_[index];
  else
    return NULL;
}

void Filename::Initialize(const char* file) {
  string file_str(file);
  vector<string> dirs;
  int pos_begin = 0;
  int pos_end = file_str.find_first_of("/");

  CHECK_NE(file_str, "");

  // Find components and keep them in a vector
  while (pos_end != -1) {
    string dir = file_str.substr(pos_begin, pos_end - pos_begin);
    if (dir != ".")  // Ignore directories just named "."
      dirs.push_back(dir);

    pos_begin = pos_end + 1;
    pos_end = file_str.find_first_of("/", pos_begin);
  }

  string dir = file_str.substr(pos_begin, file_str.length() - pos_begin);
  if (dir != ".")
    dirs.push_back(dir);
  if (dirs.size() == 0)
    dirs.push_back("");

  // Allocate space and put components into SymbolTable
  const char ** filetmp = new const char*[dirs.size()];
  filename_ = filetmp;
  for (int i = 0; i < dirs.size(); ++i, ++filetmp)
    *filetmp = GetString(dirs[i].c_str());

  subdirs_ = dirs.size();
}
