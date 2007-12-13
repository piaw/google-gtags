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

#include "sexpression_util.h"
#include "sexpression.h"

// Given an S-Expression in the form of ((key1 value1) (key2 value2) ...)
// and a key, return the SExpression value associated with the key.
const SExpression* SExpressionAssocGet(const SExpression* sexpr,
                                       const string& key) {
  if (!sexpr->IsList() || sexpr->IsNil()) {
    return NULL;
  }

  SExpression::const_iterator i = sexpr->Begin();
  while (i != sexpr->End()) {
    if (!i->IsList()) {
      ++i;
      continue;
    }
    // if i is a list, check the first element against the key.
    SExpression::const_iterator j = i->Begin();
    if (j != i->End() && j->IsSymbol()) {
      const SExpressionSymbol* symbol =
          down_cast<const SExpressionSymbol*>(&(*j));
      if (symbol->name() == key) {
        ++j;
        return &(*j);
      }
    }
    ++i;
  }
  return NULL;
}

string SExpressionAssocReplace(const SExpression* sexpr, const string& key,
                               const string& value) {
  if (!sexpr->IsList() || sexpr->IsNil()) {
    return sexpr->Repr();
  }

  string output = "(";
  for (SExpression::const_iterator i = sexpr->Begin(); i != sexpr->End();
       ++i) {
    if (!i->IsList()) {
      output.append(i->Repr());
      output.append(" ");
      continue;
    }
    // If i is a list, check the first element against the key.
    SExpression::const_iterator j = i->Begin();
    if (j != i->End() && j->IsSymbol()) {
      const SExpressionSymbol* symbol =
          down_cast<const SExpressionSymbol*>(&(*j));
      if (symbol->name() == key) {  // Found key, do substitution.
        output.append("(");
        output.append(key);
        output.append(" ");
        output.append(value);
        output.append(") ");
        continue;
      }
    }

    // If i was a list or it didn't match the key, add i to output.
    output.append(i->Repr());
    output.append(" ");
  }
  output.append(")");
  return output;
}
