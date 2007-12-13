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
// SExpression implements an immutable representation for storing Lisp
// s-expressions.
//
// SExpression::Parse() parses a string containing an s-expression and
// returns a corresponding SExpression object, and the Repr() method
// of any SExpression object gives the printed representation in
// canonical form as a string.
//
// This implementation supports reading and printing s-expressions
// which may contain integers (in the compiler's int range and given
// in decimal), quoted strings, symbols, pairs, and lists. This makes
// the set of readable s-expressions a subset of those allowable by
// Common Lisp (Common Lisp the Language, 2nd Edition:
// <http://www.cs.cmu.edu/Groups/AI/html/cltl/cltl2.html>). Notably,
// we do not support integers with radices other than 10, integers
// outside the int range (bignums), floating-point numbers, ratios,
// characters (denoted like #\g), or the quote operator. Also, we do
// not automatically uppercase symbol names, as a CL reader does.
//
// An s-expression is any one of the following:
//
// * An integer in the int range of the C++ compiler, represented
//   verbatim in decimal. All integers must match the regular
//   expression [+-]?[0-9]+.
//
// * A string, represented by surrounding the string with double quote
//   marks '"'. Backslashes '\' and '"' which are inside the string
//   must be escaped by preceding them with a backquote.
//
// * A symbol, which is identified by an arbitrary string, called the
//   'symbol name'. In general, a symbol may be represented by
//   surrounding its name with bars '|'. Backslashes '\' and bars '|'
//   which are inside the symbol name must be escaped by preceding
//   them with a backquote. If the symbol consists only of
//   alphanumeric characters and the pseudoalphabetic characters
//
//              + - * / @ $ % ^ & _ = < > ~ . ? ! [ ] { }
//
//   and it does not consist only of periods '.', and the symbol
//   cannot be construed as an integer (see the integer regexp above),
//   then the symbol can be represented by printing it verbatim,
//   without bars.
//
// * A pair, which contains pointers to two other s-expressions,
//   called the car and the cdr. It may be represented as (A . B)
//   where A and B are the representations of the car and the cdr,
//   respectively.
//
// The above definitions allow the construction of an equivalent to
// any s-expression allowed under our implementation, but we allow the
// following special syntax to create the complex nested-pair
// structures which are common in Lisp:
//
// * A list is represented by (A B ... N) and may contain any number
//   of elements. When read, the structure created is the same as that
//   created by
//
//       (A . (B . ... (N . nil)...))
//
//   More formally: the empty list () is read as the symbol nil. For
//   any non-empty list (A B ... N), the representation is the same as
//   if we had read a pair with car containing A and cdr containing
//   the list formed from the rest of the elements:
//
//       (A B C ... N) === (A . (B C ... N))
//
// * An improper list is represented by (A B ... M . N). There need to
//   be at least two elements. When read, the structure created is the
//   same as that created by
//
//       (A . (B . ... (M . N)...))
//
//   More formally: an improper list with two elements (A . B) is read
//   as a pair. For any improper list with three or more elements, the
//   representation is the same as if we had read a pair with car
//   containing the first element and cdr containing the improper list
//   formed from the remaining elements:
//
//       (A B ... M . N) === (A . (B ... M . N))
//
// Whenever an s-expression is a pair whose cdr is either nil or
// another pair, the s-exp is printed as a list or improper list
// (depending on whether the chain of cdrs is terminated by nil or
// some other s-exp), rather than as nested pairs, e.g. (1 2 3)
// instead of (1 . (2 . (3 . nil))).

#ifndef TOOLS_TAGS_SEXPRESSION_H__
#define TOOLS_TAGS_SEXPRESSION_H__

#include <string>

#include "iterators.h"
#include "tagsutil.h"

// Need forward declaration for Pair since it's used in the internal
// representation of SExpression::const_iterator
class SExpressionPair;

// Need forward declaration for typedef
class SExpressionConstIterator;

// SExpression is an abstract class, instances of which represent a
// single s-expression. It is the superclass for all classes
// representing specific types of s-exps.
class SExpression {
 public:
  virtual ~SExpression() { }

  // Appends a printed representation of this s-exp to str.
  virtual void WriteRepr(string* str) const = 0;

  // Returns the printed representation as a string.
  virtual string Repr() const;

  // Tests for various types of s-exps. Implementing classes should
  // override them as appropriate.
  virtual bool IsPair()    const { return false; }
  virtual bool IsNil()     const { return false; }
  virtual bool IsList()    const { return false; }  // nil, or pair
                                                    // with a list as
                                                    // a cdr
  virtual bool IsAtom()    const { return !IsPair(); }
  virtual bool IsSymbol()  const { return false; }
  virtual bool IsString()  const { return false; }
  virtual bool IsInteger() const { return false; }

  // Returns a new SExpression corresponding to the first s-exp in the
  // given string. Returns NULL if no complete s-exp is found.
  static SExpression* Parse(const char*);
  static SExpression* Parse(const string& str) {
    return Parse(str.c_str());
  }

  typedef SExpressionConstIterator const_iterator;

  // Return iterators for beginning and end of list, provided this
  // SExpression is actually a list
  const_iterator Begin() const;
  const_iterator End() const;

 protected:
  // Default constructor. This is empty since there's no
  // initialization needed which is common to all SExpression objects.
  SExpression() { }

  // Returns true if the string can be interpreted as an int without
  // leading or trailing whitespace.
  static bool TokenIsInteger(const char* str);

  // Returns true if the string is all periods (such strings are not
  // valid symbol names).
  static inline bool TokenIsAllPeriods(const char* str) {
    while (*str) {
      if (*str != '.')
        return false;
      ++str;
    }
    return true;
  }

  // Returns the first complete SExpression parsed from the text of
  // the CharacterIterator. Advances the iterator to point to the
  // first character after the s-exp. Returns NULL if no complete
  // s-exp is found.
  static SExpression* ParseSexp(CharacterIterator*);

  // ParseFromCharInterator is required by FileReader template class.
  static SExpression* ParseFromCharIterator(CharacterIterator* iter) {
    return ParseSexp(iter);
  }

 private:
  // FileReader needs access to SExpression's private parsing methods
  friend class FileReader<SExpression>;

  // Returns a new SExpression corresponding to the first s-exp of the
  // given type in the CharacterIterator. Assumes there is no leading
  // whitespace. Modifies the iterator pointer to point to the first
  // character after the s-exp. If no complete s-exp of the given type
  // is found, we return NULL, and the position of the iterator is
  // unspecified.

  // Pairs, lists, and dotted (improper) lists
  static SExpression* ParseList(CharacterIterator*);
  // Quoted strings like "this"
  static SExpression* ParseString(CharacterIterator*);
  // Symbols in bar notation like |this one|
  static SExpression* ParseSymbolInBars(CharacterIterator*);
  // Symbols not in bars, and numeric literals
  static SExpression* ParseUnquotedToken(CharacterIterator*);

  // Finds the string in *psexp bounded by the specified delimiter and
  // appends it to str. Advances the pointer to point to the first
  // character after the expression. Returns true if the string is
  // extracted successfully (false if the string ends before a closing
  // delimiter is found).
  //
  // In particular, when delimiter is '"', this function gets the
  // contents of a quoted string from the input string:
  // e.g. FillWithDelimitedString('"', "\"quoted\" other stuff", str)
  //        puts "quoted" into str
  // e.g. FillWithDelimitedString('|', "|symbol|", str)
  //        puts "symbol" into str
  static bool FillWithDelimitedString(char delimiter,
                                      CharacterIterator* psexp,
                                      string* str);

  // Returns an int corresponding to the string, provided IsInteger is
  // true and the value is within the int range.
  static int IntegerValue(const char* str);

  DISALLOW_EVIL_CONSTRUCTORS(SExpression);
};

// SExpressionConstIterator supports one-way read-only iteration. If psexp
// points to a list, the iter goes to each SExpression in the list,
// in order.
//
// Sample usage:
//
// for(SExpression::const_iterator iter = my_list->Begin();
//     iter != my_list->End();
//     ++iter)
//   str.append(iter->Repr());
class SExpressionConstIterator {
 public:
  // Construct an iterator for the given list, provided this
  // SExpression is actually a list
  explicit SExpressionConstIterator(const SExpression* psexp);
  // Construct an empty iterator (useful for generating sexp.End())
  explicit SExpressionConstIterator() : next_(NULL), done_(true) {}

  // Dereference pointer to current SExpression
  const SExpression& operator*() const;
  const SExpression* operator->() const;

  // Support == and != for use in for-loop constructs
  inline bool operator==(const SExpressionConstIterator& iter) const {
    return (done_ == iter.done_) && (next_ == iter.next_);
  }

  inline bool operator!=(const SExpressionConstIterator& iter) const {
    return !(*this == iter);
  }

  // Pre-increment advances to next element
  void operator++();

 private:
  // The iterator holds on to the SExpressionPair whose car is the
  // next item, and advances to the cdr with each increment.

  // Stores the next item we want to visit, if there are items left
  const SExpressionPair* next_;
  // True when there are no more items
  bool done_;
};

// SExpressionNil represents nil or ().
//
// Nil is immutable and all instances are functionally
// indistinguishable. We could save space by turning it into a
// singleton, but this would break delete. Here are some possible
// fixes:
//
// (1) Require that clients check to see if an SExpression is Nil
//     before deleting it.
// (2) Require that clients call a method DeleteSexp(ptr) instead of
//     simply "delete ptr".
// (3) Overload delete so that it doesn't delete Nil objects.
//
// However, 1 and 2 makes usage of SExpression objects different
// between sub-types, and 3 seems to not work well with our debug
// libraries (as well as being against the style policies).
//
// Consequently, Nil is NOT a singleton; pointers to SExpressionNil
// objects may point to different addresses, and comparing Nil objects
// should be done with a new Equals() function (however, the same is
// true of pointers to SExpressionSymbol objects with the same name).
class SExpressionNil : public SExpression {
 public:
  SExpressionNil() { }

  // In this implementation, the canonical representation of
  // empty-list or nil is always 'nil', not '()'
  virtual void WriteRepr(string* str) const {
    str->append("nil");
  }

  virtual bool IsNil() const    { return true; }
  virtual bool IsList() const   { return true; }
  virtual bool IsSymbol() const { return true; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SExpressionNil);
};

// SExpressionPair represents a Lisp pair (also known as a cons).
//
// An SExpressionPair is a data structure which points to two other
// SExpressions, called the car and the cdr.
//
// SExpressionPair objects are used to represent lists in the usual
// way (see definitions at top of file).
class SExpressionPair : public SExpression {
 public:
  // New pair with the specified car and cdr. The car and the cdr will
  // be deleted when the pair goes away.
  SExpressionPair(SExpression* car, SExpression* cdr) : car_(car), cdr_(cdr) {}

  ~SExpressionPair() {
    delete car_;
    delete cdr_;
  }

  // Retrieves either of the referenced SExpressions
  inline SExpression* car() const { return car_; }
  inline SExpression* cdr() const { return cdr_; }

  virtual void WriteRepr(string* str) const;

  virtual bool IsPair() const { return true; }
  virtual bool IsList() const { return cdr_->IsList(); }

  // Append the printed representation, without the outermost
  // parentheses, to the given string.
  void WriteReprWithoutParens(string* str) const;

 private:
  // Pointers for the car and cdr cells of this pair.
  SExpression* const car_;
  SExpression* const cdr_;

  DISALLOW_EVIL_CONSTRUCTORS(SExpressionPair);
};

// SExpressionSymbol represents any Lisp symbol other than nil.
class SExpressionSymbol : public SExpression {
 public:
  // Creates a new symbol with the specified name (given as a string).
  explicit SExpressionSymbol(const string&);

  virtual void WriteRepr(string* str) const;

  virtual bool IsSymbol() const { return true; }

  const string& name() const { return name_; }

 private:
  // kAsciiSymbol has an 'x' at each position corresponding to an
  // ASCII code which can be printed in a symbol without that symbol
  // requiring bar notation. These include alphanumeric chars and the
  // pseudoalphabetic characters,
  //
  //          + - * / @ $ % ^ & _ = < > ~ . ? ! [ ] { }
  static const char kAsciiSymbol[256];

  // Determines if c is one of the characters (described above) which
  // can be printed in a symbol without special treatment.
  static inline bool IsSymbolChar(unsigned char c) {
    return kAsciiSymbol[c] == 'x';
  }

  // Stores the symbol name as a string
  const string name_;

  // Whether or not the symbol needs to be printed in |bar|
  // notation. This happens if it could be misconstrued as a number,
  // contains only periods, or contains characters other than
  // alphanumeric chars and +-*/@$%^&_=<>~.?![]{}
  bool needs_quoting_;

  DISALLOW_EVIL_CONSTRUCTORS(SExpressionSymbol);
};

// SExpressionString represents a string.
class SExpressionString : public SExpression {
 public:
  // Creates a new string with the specified value.
  explicit SExpressionString(const string& value) : value_(value) {}

  // Prints string with c-style escapes.
  virtual void WriteRepr(string* str) const {
    str->push_back('"');
    str->append(CEscape(value_));
    str->push_back('"');
  }

  virtual bool IsString() const { return true; }

  const string& value() const { return value_; }

 private:
  // Stores the string value.
  const string value_;

  DISALLOW_EVIL_CONSTRUCTORS(SExpressionString);
};

// SExpressionInteger represents an integer in the int range.
class SExpressionInteger : public SExpression {
 public:
  // Creates a new integer with specified value.
  explicit SExpressionInteger(int value) : value_(value) {}

  virtual void WriteRepr(string* str) const {
    str->append(FastItoa(value_));
  }

  virtual bool IsInteger() const { return true; }

  int value() const { return value_; }

 private:
  // Stores the integer value.
  const int value_;

  DISALLOW_EVIL_CONSTRUCTORS(SExpressionInteger);
};

#endif  // TOOLS_TAGS_SEXPRESSION_H__
