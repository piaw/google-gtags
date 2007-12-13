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

#ifndef TOOLS_TAGS_ITERATORS_H__
#define TOOLS_TAGS_ITERATORS_H__

#include "strutil.h"
#include "tagsutil.h"

// CharacterIterator specifies an interface for one-way const
// iteration over a set of characters. It's used as a generic input
// source for SExpression::ParseSexp. We guarantee that the stream
// returned by the CharacterIterator contains zero or more non-\0
// characters, followed by a \0, and IsDone() is true only when the
// cursor is at \0.
//
// We provide, below, implementations to iterate through strings and
// files.
//
// Sample usage:
//
// for (CharacterIterator* c_iter = new ...; !c_iter->IsDone(); ++c_iter) {
//   char c = *c_iter;
//   ...
// }
class CharacterIterator {
 public:
  virtual ~CharacterIterator() { }

  // Dereferences the iterator to a character.
  virtual const char operator*() const = 0;
  // Advances to the next character in the sequence.
  virtual void operator++() = 0;
  // Returns true if no more characters can be read. If IsDone(), ++
  // has undefined effects. IsDone() should be synonymous with
  // *char_iter == '\0'.
  virtual bool IsDone() const = 0;

 protected:
  CharacterIterator() {}
};

// Simple class to support iterating over the characters of a c-str.
class StringCharacterIterator : public CharacterIterator {
 public:
  // Creates a new iterator over the given c-str.
  explicit StringCharacterIterator(const char* str)
      : CharacterIterator(),
        cursor_(str) {}

  virtual const char operator*()  const { return *cursor_; }
  virtual void operator++() { ++cursor_; }
  virtual bool IsDone() const { return cursor_ == '\0'; }

 private:
  // Points to the next character to read.
  const char* cursor_;
};

// Abstract class that iterates over the characters read from a file
// object. Implementing classes can set the file object to take
// input from different sources.
class FileObjectCharacterIterator : public CharacterIterator {
 public:
  // Closes the file when we're done.
  virtual ~FileObjectCharacterIterator() { fclose(file_); }

  virtual const char operator*() const {
    return (current_char_ == EOF) ? '\0' : current_char_;
  }
  virtual void operator++() { LoadNextChar(); }
  virtual bool IsDone() const { return current_char_ == EOF; }

 protected:
  // Implementing classes should call Init() in their constructors
  // and pass in the file object to be read.
  explicit FileObjectCharacterIterator() : CharacterIterator() {}

  void Init(FILE* file) {
    file_ = file;
    LoadNextChar();
  }

 private:
  // Gets the next char from file, checking to see there was not a
  // read error.
  inline void LoadNextChar() {
    current_char_ = fgetc(file_);
    // Signal when there is a file error
    CHECK(!(current_char_ == EOF && ferror(file_)));
  }

  // FILE object we're reading from
  FILE* file_;
  // Always contains the next character to return
  int current_char_;
};

// Iterates over the characters of a file.
class FileCharacterIterator : public FileObjectCharacterIterator {
 public:
  // Creates a new iterator reading from the specified file.
  explicit FileCharacterIterator(string filename)
      : FileObjectCharacterIterator() {
    FILE * file = fopen(filename.c_str(), "r");
    CHECK(file != NULL) << "Could not open file " << filename;
    Init(file);
  }

  virtual ~FileCharacterIterator() { }
};

// gunzips a file stream and reads the characters
class GzippedFileCharacterIterator : public FileObjectCharacterIterator {
 public:
  // Creates a new iterator reading from the specified file.
  explicit GzippedFileCharacterIterator(string filename)
      : FileObjectCharacterIterator() {
    //Check file existance
    FILE * file = fopen(filename.c_str(), "r");
    CHECK(file != NULL);
    fclose(file);

    // Pipe in the output from "gunzip -c FILENAME".
    string cmd;
    cmd.append("gunzip -c ");
    cmd.append(filename.c_str());

    Init(popen(cmd.c_str(), "r"));
  }

  virtual ~GzippedFileCharacterIterator() { }
};

// Moves *psexp so that it points to the first non-whitespace
// pointer in the string.
template <typename T>
static inline void SkipWhitespace(T* psexp) {
  while (ascii_isspace(**psexp))
    ++(*psexp);
}

// FileReader reads the s-expressions from a file sequentially. Sample
// usage:
//
// SExpression::FileReader f("/path/to/file");
// while (!f.IsDone()) {
//   SExpression* s = f.GetNext();
//   ...
// }
template<class T>
class FileReader {
 public:
  // Creates new reader reading from the specified file.
  FileReader(string filename)
      : pchar_iter(new FileCharacterIterator(filename)) {}

  // Creates new reader reading from the specified file, optionally
  // enabling a gunzip filter on the input.
  FileReader(string filename, bool enable_gunzip) {
    if (enable_gunzip)
      pchar_iter = new GzippedFileCharacterIterator(filename);
    else
      pchar_iter = new FileCharacterIterator(filename);
  }

  // Deletes the underlying iterator when we're done.
  ~FileReader() { delete pchar_iter; }

  // Returns the next s-expression found in the file.
  T* GetNext() { return T::ParseFromCharIterator(pchar_iter); }

  // Returns true if there is nothing more to read from the file.
  bool IsDone() {
    // Skip over whitespace so we can tell whether there are
    // subsequent s-expressions, or just EOF.
    SkipWhitespace(pchar_iter);
    return pchar_iter->IsDone();
  }

 private:
  // This is the underlying iterator used to get all the characters in
  // the file.
  CharacterIterator* pchar_iter;
};

#endif  // TOOLS_TAGS_ITERATORS_H__
