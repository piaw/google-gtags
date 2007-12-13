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
//
// Utility functions for reading SExpression lists.

#ifndef TOOLS_TAGS_SEXPRESSION_UTIL_INL_H__
#define TOOLS_TAGS_SEXPRESSION_UTIL_INL_H__

#include <ext/hash_map>

#include "sexpression.h"

// Generic wrappers for converting SExpression to the actual data they
// represent. These are needed for generic helper functions such as
// ReadList and ReadPairList.
template <typename T>
struct Type {
  typedef T type;
};

// Template specialization for SExpressionString to String conversion.
template<> struct Type<string> {
  static bool IsType(const SExpression* s) { return s->IsString(); }
  static const string& ToType(const SExpression* s) {
    return down_cast<const SExpressionString*>(s)->value();
  }
};

// Template specialization for SExpressionInteger to int conversion.
template<> struct Type<int> {
  static bool IsType(const SExpression* s) { return s->IsInteger(); }
  static int ToType(const SExpression* s) {
    return down_cast<const SExpressionInteger*>(s)->value();
  }
};

// Template specialization for SExpression to boolean conversion.
template<> struct Type<bool> {
  // Every s-expression can be evaluated as a boolean.
  static bool IsType(const SExpression* s) { return true; }
  static bool ToType(const SExpression* s) { return !s->IsNil(); }
};

// Generic function for reading a list of SExpressions of the same type,
// extracting the value and push them onto a list.
template<typename T>
void ReadList(SExpression::const_iterator begin,
              SExpression::const_iterator end,
              list<T>* l) {
  for (SExpression::const_iterator i = begin; i != end; ++i) {
    if (Type<T>::IsType(&(*i))) {
      l->push_back(Type<T>::ToType(&(*i)));
    }
  }
}

// Generic function for reading a list of pairs of SExpression of the same type,
// extracting the values and add the pair as key/value to a hash_map.
template<typename T1, typename T2>
void ReadPairList(SExpression::const_iterator begin,
                  SExpression::const_iterator end,
                  hash_map<T1, T2>* m) {
  for (SExpression::const_iterator i = begin; i != end; ++i) {
    if (i->IsPair()) {
      const SExpressionPair* p = down_cast<const SExpressionPair*>(&(*i));
      if (Type<T1>::IsType(p->car()) && Type<T2>::IsType(p->cdr())) {
        (*m)[Type<T1>::ToType(p->car())] = Type<T2>::ToType(p->cdr());
      }
    }
  }
}

#endif  // TOOLS_TAGS_SEXPRESSION_UTIL_INL_H__
